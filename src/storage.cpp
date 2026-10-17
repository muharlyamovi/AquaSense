#include "storage.h"
#include <Preferences.h>
#include <sys/time.h>

Settings S;
RTC_DATA_ATTR Counters C;

static const char *NS = "aqua";

static void setSystemTime(uint32_t epoch) {
  struct timeval tv = {(time_t)epoch, 0};
  settimeofday(&tv, nullptr);
}

uint32_t nowEpoch() { return (uint32_t)time(nullptr); }
bool timeValid() { return nowEpoch() > TIME_VALID_MIN; }

void settingsApplyPreset(uint8_t type) {
  S.filterType = type;
  if (type == FILTER_RO) {
    S.tdsT1 = 30; S.tdsT2 = 80; S.tdsT3 = 150; S.tdsRise = 2.0f;
  } else {
    // угольный фильтр почти не меняет TDS, поэтому рост TDS не считается признаком износа
    S.tdsT1 = 150; S.tdsT2 = 300; S.tdsT3 = 600; S.tdsRise = 0.0f;
  }
}

static void settingsDefaults() {
  memset(&S, 0, sizeof(S));
  S.version = SETTINGS_VERSION;
  strlcpy(S.devName, "AquaSense", sizeof(S.devName));
  strlcpy(S.model, "Картридж RO", sizeof(S.model));
  strlcpy(S.shopUrl, DEFAULT_SHOP_URL, sizeof(S.shopUrl));
  S.litersPerPulse = 1.0f;
  S.cartridgeLiters = 3000;
  S.cartridgeMonths = 6;
  S.tdsK = 1.0f;
  S.autoOta = 1;
  S.otaHours = 24;
  strlcpy(S.otaUrl, DEFAULT_OTA_URL, sizeof(S.otaUrl));
  strlcpy(S.ghRepo, DEFAULT_GH_REPO, sizeof(S.ghRepo));
  settingsApplyPreset(FILTER_RO);
}

void storageLoadSettings() {
  Preferences p;
  bool ok = false;
  if (p.begin(NS, true)) {
    size_t n = p.getBytesLength("set");
    if (n > 0 && n <= sizeof(S)) {
      memset(&S, 0, sizeof(S));
      ok = p.getBytes("set", &S, n) == n && S.version >= 1;
      if (ok && (n < sizeof(S) || S.version < SETTINGS_VERSION)) {
        if (!S.otaUrl[0]) strlcpy(S.otaUrl, DEFAULT_OTA_URL, sizeof(S.otaUrl));
        if (n < sizeof(S)) { S.autoOta = 1; S.otaHours = 24; }
        if (!S.otaHours) S.otaHours = 24;
        S.version = SETTINGS_VERSION;
      }
    }
    p.end();
  }
  if (!ok || S.version != SETTINGS_VERSION) settingsDefaults();
  if (!S.otaUrl[0]) strlcpy(S.otaUrl, DEFAULT_OTA_URL, sizeof(S.otaUrl));
  if (!S.ghRepo[0]) strlcpy(S.ghRepo, DEFAULT_GH_REPO, sizeof(S.ghRepo));
}

void storageSaveSettings() {
  Preferences p;
  p.begin(NS, false);
  p.putBytes("set", &S, sizeof(S));
  p.end();
}

void storageLoadCounters() {
  Preferences p;
  bool ok = false;
  if (p.begin(NS, true)) {
    ok = p.getBytesLength("cnt") == sizeof(C) && p.getBytes("cnt", &C, sizeof(C)) == sizeof(C);
    p.end();
  }
  if (ok) {
    C.magic = COUNTERS_MAGIC;
    // после пропадания питания часы стартуют с 1970 — продолжаем с последнего сохранённого момента
    if (C.lastEpoch > nowEpoch()) setSystemTime(C.lastEpoch);
    LOG("counters restored from NVS, pulses=%u\n", C.totalPulses);
    return;
  }
  memset(&C, 0, sizeof(C));
  C.magic = COUNTERS_MAGIC;
  uint32_t now = max<uint32_t>(nowEpoch(), 1);
  C.installEpoch = now;
  C.lastDailyEpoch = now;
  LOG("counters initialized\n");
}

void storageSaveCounters() {
  C.lastEpoch = nowEpoch();
  C.savedPulses = C.totalPulses;
  Preferences p;
  p.begin(NS, false);
  p.putBytes("cnt", &C, sizeof(C));
  p.end();
}

void storageLoadHistory(History &h) {
  memset(&h, 0, sizeof(h));
  Preferences p;
  if (p.begin(NS, true)) {
    if (p.getBytesLength("hist") == sizeof(h)) p.getBytes("hist", &h, sizeof(h));
    p.end();
  }
  if (h.head >= HISTORY_DAYS || h.count > HISTORY_DAYS) memset(&h, 0, sizeof(h));
}

static void storageSaveHistory(const History &h) {
  Preferences p;
  p.begin(NS, false);
  p.putBytes("hist", &h, sizeof(h));
  p.end();
}

void storageAddDay(const DayRecord &r) {
  History h;
  storageLoadHistory(h);
  h.d[h.head] = r;
  h.head = (h.head + 1) % HISTORY_DAYS;
  if (h.count < HISTORY_DAYS) h.count++;
  storageSaveHistory(h);
}

void storageFactoryReset() {
  Preferences p;
  p.begin(NS, false);
  p.clear();
  p.end();
  memset(&C, 0, sizeof(C));
}

void shiftStoredEpochs(int64_t delta) {
  auto sh = [delta](uint32_t &v) { if (v) v = (uint32_t)((int64_t)v + delta); };
  sh(C.installEpoch);
  sh(C.lastTdsEpoch);
  sh(C.lastEpoch);
  sh(C.lastDailyEpoch);
  sh(C.lastNtpEpoch);
  sh(C.lastNtpTryEpoch);
  sh(C.lastReminderEpoch);
  History h;
  storageLoadHistory(h);
  for (auto &d : h.d) sh(d.epoch);
  storageSaveHistory(h);
  storageSaveCounters();
}

void applyTime(uint32_t epoch) {
  uint32_t cur = nowEpoch();
  if ((int64_t)epoch - cur < 30 && (int64_t)cur - epoch < 30) return;
  bool wasValid = timeValid();
  setSystemTime(epoch);
  // пока время не было известно, все метки шли от условного нуля — переносим их на реальную шкалу
  if (!wasValid && epoch > TIME_VALID_MIN) shiftStoredEpochs((int64_t)epoch - cur);
}

String deviceId() {
  uint64_t mac = ESP.getEfuseMac();
  char b[8];
  snprintf(b, sizeof(b), "%06X", (unsigned)((mac >> 24) & 0xFFFFFF));
  return String(b);
}
