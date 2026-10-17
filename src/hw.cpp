#include "hw.h"
#include "config.h"
#include "storage.h"
#include "driver/gpio.h"
#include <Wire.h>

#define MAX17043_ADDR  0x36
#define REG_VCELL      0x02
#define REG_VERSION    0x08
#define REG_CONFIG     0x0C
#define CONFIG_SLEEP   0x0080

static bool wireStarted = false;

void hwInitPins() {
  gpio_hold_dis((gpio_num_t)PIN_SENS_PWR);
  sensorsPower(false);
  pinMode(PIN_LED, OUTPUT);
  ledSet(false);
#if BUZZER_ENABLED
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
#endif
}

void hwPrepareSleep() {
  sensorsPower(false);
  // без внешней подтяжки затвор иначе всплывёт во сне и MOSFET снова откроется
  gpio_hold_en((gpio_num_t)PIN_SENS_PWR);
  ledSet(false);
  pinMode(PIN_LED, INPUT);
  if (wireStarted) gaugeSleep();
}

// ---------------- Питание датчиков ----------------
// GPIO2 = затвор P-MOSFET: LOW открывает (датчики вкл), HIGH закрывает (выкл).
// Раньше «выкл» ставил пин во вход — без резистора 100k затвор плавал и TDS оставался включённым.

void sensorsPower(bool on) {
  gpio_hold_dis((gpio_num_t)PIN_SENS_PWR);
#if SENS_PWR_VIA_PMOS
  pinMode(PIN_SENS_PWR, OUTPUT);
  digitalWrite(PIN_SENS_PWR, on ? LOW : HIGH);
#else
  pinMode(PIN_SENS_PWR, OUTPUT);
  digitalWrite(PIN_SENS_PWR, on ? HIGH : LOW);
#endif
}

void hwPowerCheck(PowerCheck &p) {
  sensorsPower(false);
  delay(80);
  p.tdsOff = analogReadMilliVolts(PIN_TDS_ADC);
  p.ntcOff = analogReadMilliVolts(PIN_NTC_ADC);
  sensorsPower(true);
  delay(SENSOR_WARMUP_MS);
  p.tdsOn = analogReadMilliVolts(PIN_TDS_ADC);
  p.ntcOn = analogReadMilliVolts(PIN_NTC_ADC);
  sensorsPower(false);
  delay(80);
  int tdsAfter = analogReadMilliVolts(PIN_TDS_ADC);
  p.switched = tdsAfter < 20 && p.ntcOff < 20;
  LOG("pwr: off tds=%dmV ntc=%dmV | on tds=%dmV ntc=%dmV | after=%dmV %s\n",
      p.tdsOff, p.ntcOff, p.tdsOn, p.ntcOn, tdsAfter, p.switched ? "SWITCH OK" : "TDS STAYS ON");
}

// ---------------- TDS + NTC ----------------

static int cmpInt(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

static int median(int *v, int n) {
  qsort(v, n, sizeof(int), cmpInt);
  return v[n / 2];
}

float tdsFromMv(float mv, float tempC, float k) {
  // формула DFRobot Gravity TDS с термокомпенсацией 2 %/°C
  float v = mv / 1000.0f;
  float cv = v / (1.0f + 0.02f * (tempC - 25.0f));
  float tds = (133.42f * cv * cv * cv - 255.86f * cv * cv + 857.39f * cv) * 0.5f * k;
  return tds < 0 ? 0 : tds;
}

bool waterMeasure(WaterReading &r) {
  memset(&r, 0, sizeof(r));
  int tdsMv[ADC_SAMPLES], ntcMv[ADC_SAMPLES];

  sensorsPower(true);
  delay(SENSOR_WARMUP_MS);
  for (int i = 0; i < ADC_SAMPLES; i++) {
    tdsMv[i] = analogReadMilliVolts(PIN_TDS_ADC);
    ntcMv[i] = analogReadMilliVolts(PIN_NTC_ADC);
    delay(25);
  }
  sensorsPower(false);

  r.tdsMv = median(tdsMv, ADC_SAMPLES);
  r.ntcMv = median(ntcMv, ADC_SAMPLES);

  // NTC сверху (к питанию), 10 кОм снизу (к GND): R_ntc = R_fixed * (Vcc - V) / V
  r.tempC = 25.0f;
  r.ntcOk = r.ntcMv > 50 && r.ntcMv < VCC_MV - 50;
  if (r.ntcOk) {
    float rntc = NTC_R_FIXED * (VCC_MV - r.ntcMv) / r.ntcMv;
    float t = 1.0f / (1.0f / 298.15f + logf(rntc / NTC_R25) / NTC_BETA) - 273.15f;
    if (t > -10 && t < 90) r.tempC = t;
    else r.ntcOk = false;
  }

  r.dry = r.tdsMv < TDS_DRY_MV;
  r.tdsRaw = tdsFromMv(r.tdsMv, r.tempC, 1.0f);
  r.tds = r.tdsRaw * S.tdsK;
  r.ok = !r.dry;
  LOG("water: tds=%dmV -> %.1f ppm, ntc=%dmV -> %.1f C%s\n", r.tdsMv, r.tds, r.ntcMv, r.tempC, r.dry ? " (dry)" : "");
  return r.ok;
}

// ---------------- MAX17043 ----------------

static bool rd16(uint8_t reg, uint16_t &v) {
  Wire.beginTransmission(MAX17043_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MAX17043_ADDR, (uint8_t)2) != 2) return false;
  uint8_t hi = Wire.read();
  uint8_t lo = Wire.read();
  v = ((uint16_t)hi << 8) | lo;
  return true;
}

static bool wr16(uint8_t reg, uint16_t v) {
  Wire.beginTransmission(MAX17043_ADDR);
  Wire.write(reg);
  Wire.write(v >> 8);
  Wire.write(v & 0xFF);
  return Wire.endTransmission() == 0;
}

static bool gaugeBegin() {
  if (!wireStarted) {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(100000);
    wireStarted = true;
  }
  uint16_t ver;
  return rd16(REG_VERSION, ver);
}

// возвращает true, если состояние пришлось менять
static bool gaugeSetSleep(bool sleep) {
  uint16_t cfg;
  if (!rd16(REG_CONFIG, cfg)) return false;
  if (((cfg & CONFIG_SLEEP) != 0) == sleep) return false;
  cfg = sleep ? (cfg | CONFIG_SLEEP) : (cfg & ~CONFIG_SLEEP);
  return wr16(REG_CONFIG, cfg);
}

bool gaugeRead(uint16_t &mv) {
  if (!gaugeBegin()) {
    LOG("MAX17043 not found\n");
    return false;
  }
  if (gaugeSetSleep(false)) delay(600);   // гейдж спал — ждём свежее преобразование VCELL
  uint16_t raw;
  if (!rd16(REG_VCELL, raw)) return false;
  mv = (uint16_t)((raw >> 4) * 1.25f);
  LOG("battery: %u mV\n", mv);
  return true;
}

void gaugeSleep() {
  if (gaugeBegin()) gaugeSetSleep(true);
}

uint8_t battPctFromMv(uint16_t mv) {
  // при токе покоя в десятки мкА напряжение банки практически равно ЭДС,
  // поэтому процент считаем по OCV-кривой Li-ion, а не по SOC-модели гейджа
  static const uint16_t V[] = {3300, 3450, 3550, 3650, 3700, 3750, 3800, 3850, 3920, 4000, 4100, 4200};
  static const uint8_t P[] = {0, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  if (mv <= V[0]) return 0;
  for (int i = 1; i < 12; i++) {
    if (mv <= V[i]) return P[i - 1] + (uint32_t)(P[i] - P[i - 1]) * (mv - V[i - 1]) / (V[i] - V[i - 1]);
  }
  return 100;
}

// ---------------- LED / зуммер ----------------

void ledSet(bool on) {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, (on ^ LED_ACTIVE_LOW) ? HIGH : LOW);
}

void ledBlink(uint8_t n, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < n; i++) {
    ledSet(true);
    delay(onMs);
    ledSet(false);
    if (i + 1 < n) delay(offMs);
  }
}

void beep(uint8_t n, uint16_t ms) {
#if BUZZER_ENABLED
  for (uint8_t i = 0; i < n; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(ms);
    digitalWrite(PIN_BUZZER, LOW);
    delay(ms);
  }
#else
  (void)n; (void)ms;
#endif
}
