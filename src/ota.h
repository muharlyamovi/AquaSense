#pragma once
#include <Arduino.h>

struct OtaInfo {
  bool ok;
  bool newer;
  char version[16];
  char url[192];
  char notes[160];
  char err[64];
};

bool otaConfigured();
bool otaFetch(OtaInfo &info);          // скачивает манифест; нужен домашний Wi-Fi
bool otaApply(const OtaInfo *info = nullptr);  // качает .bin и перезагружается
void otaMaybeAuto();                   // ежедневная проверка / автоустановка
