#pragma once
#include <Arduino.h>
#include "config.h"

#define SETTINGS_VERSION 2
#define COUNTERS_MAGIC   0xA0A5E001UL

enum FilterType : uint8_t { FILTER_RO = 0, FILTER_CARBON = 1 };

struct Settings {
  uint16_t version;
  char ssid[33];
  char pass[65];
  char devName[32];
  char model[48];
  char shopUrl[128];
  char tgToken[64];
  char tgChat[24];
  float litersPerPulse;
  uint32_t cartridgeLiters;
  uint16_t cartridgeMonths;
  uint8_t filterType;
  uint16_t tdsT1, tdsT2, tdsT3;   // верхние границы «отличная» / «хорошая» / «допустимая», ppm
  float tdsRise;                  // допустимый рост TDS относительно нового картриджа, раз (0 — не учитывать)
  float tdsK;                     // калибровочный коэффициент TDS
  char otaUrl[160];
  uint8_t autoOta;                // 1 — проверять и ставить самому
  uint16_t otaHours;              // интервал проверки, часов (6…168)
};

// Живёт в RTC-памяти (переживает deep sleep) и периодически копируется в NVS (переживает замену батареи).
struct Counters {
  uint32_t magic;
  uint32_t totalPulses;
  uint32_t installPulses;
  uint32_t installEpoch;
  float baselineTds, baselineSum;
  uint8_t baselineN;
  uint8_t lastFlags;              // bit0 — датчик TDS сухой, bit1 — ошибка NTC
  float tdsEma, lastTds, lastTemp;
  uint32_t lastTdsEpoch;
  uint32_t lastEpoch;
  uint32_t lastDailyEpoch;
  uint32_t lastNtpEpoch;
  uint32_t lastNtpTryEpoch;
  uint32_t lastReminderEpoch;
  uint32_t dayStartPulses;
  float dayTdsSum;
  uint16_t dayTdsN;
  uint16_t battMv;
  uint8_t battPct;
  uint8_t leakActive;
  uint8_t notifiedSoon, notifiedQuality, notifiedBatt;
  uint32_t savedPulses;
  uint32_t lastOtaCheckEpoch;
  uint32_t lastOtaNotifyVer;
};

struct DayRecord {
  uint32_t epoch;
  uint16_t liters10;
  uint16_t tds;
  uint8_t batt;
  uint8_t reserved[3];
};

struct History {
  uint8_t head, count;
  DayRecord d[HISTORY_DAYS];
};

extern Settings S;
extern Counters C;

void storageLoadSettings();
void storageSaveSettings();
void settingsApplyPreset(uint8_t type);
void storageLoadCounters();
void storageSaveCounters();
void storageLoadHistory(History &h);
void storageAddDay(const DayRecord &r);
void storageFactoryReset();

uint32_t nowEpoch();
bool timeValid();
void applyTime(uint32_t epoch);
void shiftStoredEpochs(int64_t delta);
String deviceId();
