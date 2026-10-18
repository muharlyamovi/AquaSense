#pragma once
#include <Arduino.h>

#define FW_VERSION "1.1.7"

// ===================== Пины ESP32-C3 =====================
// Из deep sleep будят только GPIO0..GPIO5, а АЦП1 есть только на GPIO0..GPIO4,
// поэтому эти шесть ножек целиком отданы под аналоговые датчики и источники пробуждения.
#define PIN_TDS_ADC     0   // ADC1_CH0 <- выход "A" модуля TDS Meter V1.0
#define PIN_NTC_ADC     1   // ADC1_CH1 <- средняя точка делителя: 10 кОм к 3V3 сверху, NTC 50 кОм к GND
#define PIN_SENS_PWR    2   // затвор P-MOSFET питания датчиков (LOW = включено); пока MOSFET нет — не использовать
#define PIN_FLOW        3   // расходомер (Холл / RC), только счёт импульсов
#define PIN_WAKE_REED   4   // геркон-будильник, активный LOW; из сна, не в объём
#define PIN_LEAK        4   // пин занят герконом, протечка выключена
#define PIN_BUTTON      5   // кнопка сервис/сброс (пробуждение, активный LOW)
#define PIN_I2C_SDA     6   // MAX17043 SDA
#define PIN_I2C_SCL     7   // MAX17043 SCL
#define PIN_LED         8   // встроенный синий LED платы ESP32-C3 SuperMini
#define LED_ACTIVE_LOW  1
#define PIN_BUZZER      10  // опционально: активный зуммер через NPN-транзистор

// 1 — датчики питаются через P-MOSFET (рекомендуется), 0 — прямо от ножки (только для отладки на столе)
#define SENS_ALWAYS_ON      1   // 1 — датчики постоянно под 3.3 В (стенд без MOSFET)
#define SENS_PWR_VIA_PMOS   0   // 1 — GPIO2 управляет P-MOSFET, когда ключ появится
#define LEAK_SENSOR_ENABLED 0   // GPIO4 занят герконом пробуждения
#define FLOW_WAKE_ENABLED   SENS_ALWAYS_ON  // пока Холл всегда под питанием — им тоже можно будить
#define BUZZER_ENABLED      0

// ===================== Тайминги =====================
#define FLOW_DEBOUNCE_MS        2   // только дребезг контакта; больше — режет частые импульсы турбины
#define FLOW_IDLE_MS            4000UL              // сколько ждать следующий импульс перед сном
#define TDS_MIN_INTERVAL_S      (20UL * 60)         // не чаще одного замера TDS за 20 минут
#define DAILY_INTERVAL_S        (24UL * 3600)       // ежедневная проверка: батарея, история, напоминания
#define LEAK_RECHECK_S          (10UL * 60)         // пока есть протечка — перепроверка по таймеру
#define NTP_INTERVAL_S          (7UL * 24 * 3600)
#define NTP_RETRY_S             (6UL * 3600)
#define REMINDER_INTERVAL_S     (3UL * 24 * 3600)   // повтор напоминания «пора менять картридж»
#define SERVICE_TIMEOUT_MS      (5UL * 60 * 1000)   // веб-интерфейс живёт 5 минут после последнего запроса
#define SETUP_TIMEOUT_MS        (10UL * 60 * 1000)
#define WIFI_CONNECT_TIMEOUT_MS 12000
#define BTN_AP_HOLD_MS          3000                // удержание 3..10 с — режим точки доступа
#define BTN_RESET_HOLD_MS       10000               // удержание >10 с — сброс до заводских
#define SAVE_EVERY_PULSES       5

// ===================== Батарея 18650 =====================
#define BATT_LOW_MV       3450
#define BATT_CRIT_MV      3250
#define BATT_SHUTDOWN_MV  3050    // ниже — спим до нажатия кнопки, чтобы не убить аккумулятор

// ===================== Датчики =====================
#define VCC_MV            3300.0f
#define NTC_R_FIXED       10000.0f  // верхний резистор к 3V3
#define NTC_R25           50000.0f  // NTC 50 кОм при 25 °C, снизу к GND
#define NTC_BETA          3950.0f
#define NTC_BOTTOM        1         // 1 — NTC к GND, 0 — NTC к питанию
#define SENSOR_WARMUP_MS  800
#define ADC_SAMPLES       21
#define TDS_DRY_MV        3         // ниже — датчик TDS не в воде
#define TDS_EMA_ALPHA     0.25f
#define BASELINE_SKIP_L   10.0f     // первые литры нового картриджа сливаются и в базовый TDS не идут
#define BASELINE_SAMPLES  5
#define BASELINE_MIN_PPM  20.0f
#define TEMP_WARN_C       30.0f

// ===================== Сеть =====================
#define AP_PASS           ""        // пусто — открытая сеть настройки, телефон сразу покажет страницу входа
#define AP_OFF_AFTER_OK_MS 20000UL
#define MDNS_NAME         "aquasense"
#define DEFAULT_SHOP_URL  "https://example.com/aquasense"
#define DEFAULT_OTA_URL   "https://raw.githubusercontent.com/muharlyamovi/AquaSense/main/ota/manifest.json"
#define DEFAULT_GH_REPO   "muharlyamovi/AquaSense"
#define OTA_INTERVAL_S    (24UL * 3600)
#define CLOUD_INTERVAL_S  (6UL * 3600)   // выгрузка статуса в GitHub, не чаще

#define HISTORY_DAYS      30
#define CONTACT_LOG_N     20
#define TIME_VALID_MIN    1700000000UL

#if AQUA_DEBUG
#define LOG(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...)
#endif
