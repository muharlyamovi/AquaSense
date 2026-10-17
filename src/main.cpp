#include <Arduino.h>
#include <WiFi.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "config.h"
#include "storage.h"
#include "hw.h"
#include "quality.h"
#include "net.h"
#include "web.h"
#include "app.h"
#include "ota.h"

static portMUX_TYPE flowMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t isrPulses = 0;
static volatile uint32_t isrLastUs = 0;
static uint32_t lastPulseMs = 0;
static String pendingMsg;

enum BtnAction { BTN_SERVICE, BTN_AP, BTN_RESET };

// ---------------- Счётчик воды ----------------

static void IRAM_ATTR onFlowEdge() {
  uint32_t t = micros();
  portENTER_CRITICAL_ISR(&flowMux);
  if (t - isrLastUs >= FLOW_DEBOUNCE_MS * 1000UL) {
    isrPulses++;
    isrLastUs = t;
  }
  portEXIT_CRITICAL_ISR(&flowMux);
}

uint32_t flowCollect() {
  portENTER_CRITICAL(&flowMux);
  uint32_t n = isrPulses;
  isrPulses = 0;
  portEXIT_CRITICAL(&flowMux);
  if (n) {
    C.totalPulses += n;
    lastPulseMs = millis();
    LOG("pulses +%u, total %u\n", n, C.totalPulses);
  }
  return n;
}

static bool tdsDue() {
  return !C.lastTdsEpoch || nowEpoch() - C.lastTdsEpoch >= TDS_MIN_INTERVAL_S;
}

// ---------------- Уведомления ----------------

String shopLink(const char *action) {
  String u = S.shopUrl;
  u += u.indexOf('?') >= 0 ? '&' : '?';
  u += "action=";
  u += action;
  u += "&device=" + deviceId();
  u += "&model=" + netUrlEncode(S.model);
  return u;
}

void notifyQueue(const String &msg) {
  LOG("notify: %s\n", msg.c_str());
  if (pendingMsg.length()) pendingMsg += "\n\n";
  pendingMsg += msg;
}

void notifyFlush() {
  if (!S.ssid[0]) { pendingMsg = ""; return; }
  if (webRunning() && webIsAp()) return;   // интернета нет, отправим после сервисной сессии

  uint32_t now = nowEpoch();
  bool tgReady = S.tgToken[0] && S.tgChat[0];
  if (!tgReady) pendingMsg = "";
  bool ntpDue = (!timeValid() || now - C.lastNtpEpoch >= NTP_INTERVAL_S) &&
                (!C.lastNtpTryEpoch || now - C.lastNtpTryEpoch >= NTP_RETRY_S);
  if (pendingMsg.isEmpty() && !ntpDue) return;

  bool wasConnected = WiFi.status() == WL_CONNECTED;
  if (!wasConnected) {
    setCpuFrequencyMhz(160);
    if (!netConnectSta(WIFI_CONNECT_TIMEOUT_MS)) {
      C.lastNtpTryEpoch = now;
      netOff();
      setCpuFrequencyMhz(80);
      pendingMsg = "";
      return;
    }
  }
  if (ntpDue) netSyncTime();
  if (pendingMsg.length()) netTelegram(String(S.devName) + ": " + pendingMsg);
  pendingMsg = "";
  if (!wasConnected) {
    netOff();
    setCpuFrequencyMhz(80);
  }
}

static void checkAlerts(const Assessment &a) {
  uint32_t now = nowEpoch();
  if (a.fstate == FS_REPLACE) {
    if (!C.lastReminderEpoch || now - C.lastReminderEpoch >= REMINDER_INTERVAL_S) {
      notifyQueue("Пора заменить картридж «" + String(S.model) + "». Износ " + String((int)a.wear) +
                  "% (фактор: " + a.reason + "). Заказать: " + shopLink("buy"));
      C.lastReminderEpoch = now;
    }
  } else if (a.fstate == FS_SOON && !C.notifiedSoon) {
    notifyQueue("Картридж скоро закончится: осталось ~" + String(a.forecastDays) + " дн. / " +
                String((int)a.litersLeft) + " л. Закажите заранее: " + shopLink("buy") +
                " или оформите подписку: " + shopLink("subscribe"));
    C.notifiedSoon = 1;
  }

  if (a.wclass == WC_BAD) {
    if (!C.notifiedQuality) {
      notifyQueue("Плохое качество воды: TDS " + String((int)a.tds) + " ppm. Проверьте фильтр.");
      C.notifiedQuality = 1;
    }
  } else if (a.wclass != WC_UNKNOWN) {
    C.notifiedQuality = 0;
  }

  if (C.battMv && C.battMv < BATT_LOW_MV) {
    if (!C.notifiedBatt) {
      notifyQueue("Низкий заряд батареи: " + String(C.battMv / 1000.0f, 2) + " В. Зарядите аккумулятор.");
      C.notifiedBatt = 1;
    }
  } else if (C.battMv > BATT_LOW_MV + 100) {
    C.notifiedBatt = 0;
  }
}

// ---------------- Батарея / протечка / кнопка ----------------

void readBattery() {
  uint16_t mv;
  if (gaugeRead(mv)) {
    C.battMv = mv;
    C.battPct = battPctFromMv(mv);
  }
}

static bool leakDetected() {
#if LEAK_SENSOR_ENABLED
  for (int i = 0; i < 3; i++) {
    if (digitalRead(PIN_LEAK) != LOW) return false;
    delay(15);
  }
  return true;
#else
  return false;
#endif
}

static void handleLeak(bool repeatAlarm) {
#if LEAK_SENSOR_ENABLED
  bool wet = leakDetected();
  if (wet && !C.leakActive) {
    C.leakActive = 1;
    storageSaveCounters();
    beep(5, 200);
    ledBlink(10, 100, 100);
    notifyQueue("ВНИМАНИЕ: обнаружена протечка воды!");
    notifyFlush();
  } else if (wet && repeatAlarm) {
    beep(3, 200);
    ledBlink(5, 100, 100);
  } else if (!wet && C.leakActive) {
    C.leakActive = 0;
    storageSaveCounters();
    notifyQueue("Протечка больше не обнаруживается.");
    notifyFlush();
  }
#else
  (void)repeatAlarm;
#endif
}

static BtnAction readButtonHold() {
  if (digitalRead(PIN_BUTTON) != LOW) return BTN_SERVICE;   // короткое нажатие уже отпущено
  uint32_t t0 = millis();
  while (digitalRead(PIN_BUTTON) == LOW) {
    uint32_t held = millis() - t0;
    if (held >= BTN_RESET_HOLD_MS) ledSet((held / 100) % 2);   // частое мигание — отпускайте для сброса
    else if (held >= BTN_AP_HOLD_MS) ledSet(true);             // горит — отпускайте для режима точки доступа
    delay(10);
  }
  ledSet(false);
  uint32_t held = millis() - t0;
  if (held >= BTN_RESET_HOLD_MS) return BTN_RESET;
  if (held >= BTN_AP_HOLD_MS) return BTN_AP;
  return BTN_SERVICE;
}

void appFactoryReset() {
  LOG("factory reset\n");
  ledBlink(10, 50, 50);
  netOff();
  storageFactoryReset();
  ESP.restart();
  while (true) {}
}

// ---------------- Сценарии пробуждения ----------------

static void serviceSession(bool forceAp) {
  setCpuFrequencyMhz(160);
  readBattery();
  webStart(forceAp);
  if (!webIsAp()) notifyQueue("Веб-интерфейс открыт: http://" + WiFi.localIP().toString() + " (5 минут)");
  uint32_t lastLeakPoll = millis(), lastBlink = 0;
  while (webLoop()) {
    flowCollect();
    if (millis() - lastLeakPoll > 1000) {
      lastLeakPoll = millis();
      handleLeak(false);
    }
    if (millis() - lastBlink > 3000) {
      lastBlink = millis();
      ledBlink(1, 20, 0);
    }
    if (!webIsAp()) notifyFlush();
    delay(2);
  }
  webStop();
  storageSaveCounters();
  setCpuFrequencyMhz(80);
}

static void handleFlowWake() {
  if (tdsDue()) measureWater();
  Assessment a = assess();
  // индикация прямо у крана: 3 вспышки — менять картридж / плохая вода, 1 — скоро замена
  if (a.fstate == FS_REPLACE || a.wclass == WC_BAD) ledBlink(3, 120, 180);
  else if (a.fstate == FS_SOON) ledBlink(1, 120, 0);
  checkAlerts(a);
}

static bool dailyDue() {
  return !C.lastDailyEpoch || nowEpoch() - C.lastDailyEpoch + 60 >= DAILY_INTERVAL_S;
}

static void dailyRoutine() {
  LOG("daily routine\n");
  readBattery();
  flowCollect();
  DayRecord r{};
  r.epoch = nowEpoch();
  r.liters10 = (uint16_t)min(65535.0f, litersToday() * 10.0f);
  r.tds = C.dayTdsN ? (uint16_t)(C.dayTdsSum / C.dayTdsN) : 0;
  r.batt = C.battPct;
  storageAddDay(r);
  C.dayStartPulses = C.totalPulses;
  C.dayTdsSum = 0;
  C.dayTdsN = 0;
  C.lastDailyEpoch = r.epoch;
  checkAlerts(assess());
  storageSaveCounters();
}

static void waitFlowIdle() {
  flowCollect();
  if (!lastPulseMs) return;
  while (millis() - lastPulseMs < FLOW_IDLE_MS) {
    if (flowCollect() && tdsDue()) measureWater();
    delay(20);
  }
}

[[noreturn]] static void goToSleep(bool critical) {
  flowCollect();
  // RTC-память переживает сон, во flash пишем реже — при замене батареи потеряется максимум пара импульсов
  if (C.totalPulses - C.savedPulses >= SAVE_EVERY_PULSES) storageSaveCounters();
  hwPrepareSleep();
  netOff();
  detachInterrupt(digitalPinToInterrupt(PIN_FLOW));

  // не засыпаем посреди импульса RC-цепочки, иначе сразу проснёмся и посчитаем его дважды
  uint32_t t0 = millis();
  while (digitalRead(PIN_FLOW) == LOW && millis() - t0 < 200) delay(1);

  uint64_t mask = 0;
  if (!critical) mask |= 1ULL << PIN_FLOW;
#if LEAK_SENSOR_ENABLED
  if (!critical && !C.leakActive) mask |= 1ULL << PIN_LEAK;
#endif
  if (digitalRead(PIN_BUTTON) == HIGH) mask |= 1ULL << PIN_BUTTON;
  if (mask) esp_deep_sleep_enable_gpio_wakeup(mask, ESP_GPIO_WAKEUP_GPIO_LOW);

  uint32_t sec = 0;
  if (!critical) {
    uint32_t now = nowEpoch();
    uint32_t elapsed = now > C.lastDailyEpoch ? now - C.lastDailyEpoch : 0;
    sec = elapsed >= DAILY_INTERVAL_S ? 60 : DAILY_INTERVAL_S - elapsed;
    if (C.leakActive) sec = min<uint32_t>(sec, LEAK_RECHECK_S);
    sec = max<uint32_t>(sec, 30);
    esp_sleep_enable_timer_wakeup((uint64_t)sec * 1000000ULL);
  }
  C.lastEpoch = nowEpoch();
  LOG("sleep %u s, wake mask 0x%llx%s\n", sec, mask, critical ? " (battery critical)" : "");
#if AQUA_DEBUG
  Serial.flush();
#endif
  esp_deep_sleep_start();
}

// ---------------- Точка входа ----------------

void setup() {
  setCpuFrequencyMhz(80);
#if AQUA_DEBUG
  Serial.begin(115200);
#endif
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  uint64_t wakeMask = cause == ESP_SLEEP_WAKEUP_GPIO ? esp_sleep_get_gpio_wakeup_status() : 0;

  for (int pin : {PIN_FLOW, PIN_LEAK, PIN_BUTTON, PIN_SENS_PWR}) gpio_hold_dis((gpio_num_t)pin);
  pinMode(PIN_FLOW, INPUT_PULLUP);
  pinMode(PIN_LEAK, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  hwInitPins();
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW), onFlowEdge, FALLING);

  storageLoadSettings();
  if (C.magic != COUNTERS_MAGIC) storageLoadCounters();

  bool coldStart = cause == ESP_SLEEP_WAKEUP_UNDEFINED;
  bool flowWake = wakeMask & (1ULL << PIN_FLOW);
  bool btnWake = (wakeMask & (1ULL << PIN_BUTTON)) || digitalRead(PIN_BUTTON) == LOW;
  LOG("wake cause=%d mask=0x%llx flow=%d leak=%d btn=%d\n", (int)cause, wakeMask,
      digitalRead(PIN_FLOW), digitalRead(PIN_LEAK), digitalRead(PIN_BUTTON));

  if (coldStart || btnWake) {
    PowerCheck pwr{};
    hwPowerCheck(pwr);
    readBattery();
    LOG("pins: flow=%d leak=%d btn=%d  batt=%u mV (%u%%)\n",
        digitalRead(PIN_FLOW), digitalRead(PIN_LEAK), digitalRead(PIN_BUTTON), C.battMv, C.battPct);
  }

  // импульс, который нас разбудил, уже закончился к этому моменту — учитываем его здесь
  if (flowWake) {
    C.totalPulses++;
    lastPulseMs = millis();
  }

  handleLeak(cause == ESP_SLEEP_WAKEUP_TIMER);

  // просадка при включении Wi-Fi — признак севшей батареи; без этой проверки получится цикл перезагрузок
  if (esp_reset_reason() == ESP_RST_BROWNOUT) goToSleep(true);
  if (C.battMv && C.battMv < BATT_SHUTDOWN_MV && !btnWake && !coldStart) goToSleep(true);

  if (btnWake || coldStart) {
    BtnAction act = btnWake ? readButtonHold() : BTN_SERVICE;
    if (act == BTN_RESET) appFactoryReset();
    serviceSession(act == BTN_AP || !S.ssid[0]);
  }

  if (flowWake) handleFlowWake();
  if (dailyDue()) dailyRoutine();
  waitFlowIdle();
  notifyFlush();
  otaMaybeAuto();
  goToSleep(false);
}

void loop() {}
