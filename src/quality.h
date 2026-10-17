#pragma once
#include <Arduino.h>
#include "hw.h"

enum WaterClass : uint8_t { WC_UNKNOWN = 0, WC_EXCELLENT, WC_GOOD, WC_FAIR, WC_BAD };
enum FilterState : uint8_t { FS_OK = 0, FS_SOON, FS_REPLACE };

struct Assessment {
  float liters, litersLeft;
  float days, daysLeft;
  float wearVol, wearTime, wearTds, wear;   // %, итоговый износ = максимум из трёх
  const char *reason;                       // какой фактор определяет износ
  FilterState fstate;
  float tds;                                // сглаженный TDS, по нему ставится класс воды
  WaterClass wclass;
  bool waterOk;
  bool sensorFault;
  bool tempWarn;
  float avgLpd;
  int forecastDays;                         // прогноз дней до замены, -1 — неизвестно
};

Assessment assess();
bool measureWater(WaterReading *out = nullptr);
void resetCartridge();
float litersSinceInstall();
float litersToday();
