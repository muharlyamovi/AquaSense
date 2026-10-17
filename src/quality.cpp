#include "quality.h"
#include "storage.h"
#include "config.h"

float litersSinceInstall() { return (C.totalPulses - C.installPulses) * S.litersPerPulse; }
float litersToday() { return (C.totalPulses - C.dayStartPulses) * S.litersPerPulse; }

static void onNewTds(const WaterReading &r) {
  C.lastTds = r.tds;
  C.lastTemp = r.tempC;
  C.tdsEma = C.tdsEma > 0 ? C.tdsEma * (1 - TDS_EMA_ALPHA) + r.tds * TDS_EMA_ALPHA : r.tds;
  C.dayTdsSum += r.tds;
  C.dayTdsN++;

  // базовый TDS нового картриджа: среднее первых замеров после промывки
  if (C.baselineTds <= 0 && litersSinceInstall() >= BASELINE_SKIP_L) {
    C.baselineSum += r.tds;
    if (++C.baselineN >= BASELINE_SAMPLES) {
      C.baselineTds = C.baselineSum / C.baselineN;
      LOG("baseline TDS = %.1f\n", C.baselineTds);
    }
  }
}

bool measureWater(WaterReading *out) {
  WaterReading r;
  waterMeasure(r);
  C.lastTdsEpoch = nowEpoch();
  C.lastFlags = (r.dry ? 1 : 0) | (r.ntcOk ? 0 : 2);
  if (r.ok) onNewTds(r);
  if (out) *out = r;
  return r.ok;
}

void resetCartridge() {
  C.installPulses = C.totalPulses;
  C.installEpoch = max<uint32_t>(nowEpoch(), 1);
  C.baselineTds = 0;
  C.baselineSum = 0;
  C.baselineN = 0;
  C.tdsEma = 0;
  C.notifiedSoon = 0;
  C.notifiedQuality = 0;
  C.lastReminderEpoch = 0;
  storageSaveCounters();
}

Assessment assess() {
  Assessment a{};
  uint32_t now = nowEpoch();

  // 1. Объём
  a.liters = litersSinceInstall();
  a.litersLeft = max(0.0f, (float)S.cartridgeLiters - a.liters);
  a.wearVol = S.cartridgeLiters ? a.liters * 100.0f / S.cartridgeLiters : 0;

  // 2. Срок службы (бактерии в угле растут даже без расхода воды)
  a.days = (C.installEpoch && now > C.installEpoch) ? (now - C.installEpoch) / 86400.0f : 0;
  float maxDays = S.cartridgeMonths * 30.44f;
  a.daysLeft = max(0.0f, maxDays - a.days);
  a.wearTime = maxDays > 0 ? a.days * 100.0f / maxDays : 0;

  // 3. Рост TDS относительно нового картриджа (имеет смысл только для обратного осмоса)
  a.tds = C.tdsEma > 0 ? C.tdsEma : C.lastTds;
  if (S.filterType == FILTER_RO && a.tds > 0) {
    if (C.baselineTds > 0 && S.tdsRise > 1.01f) {
      float base = max(C.baselineTds, BASELINE_MIN_PPM);
      a.wearTds = constrain((a.tds / base - 1.0f) / (S.tdsRise - 1.0f) * 100.0f, 0.0f, 150.0f);
    }
    if (a.tds > S.tdsT3) a.wearTds = max(a.wearTds, 100.0f);
  }

  a.wear = a.wearVol;
  a.reason = "объём";
  if (a.wearTime > a.wear) { a.wear = a.wearTime; a.reason = "срок службы"; }
  if (a.wearTds > a.wear) { a.wear = a.wearTds; a.reason = "рост TDS"; }
  a.fstate = a.wear >= 100 ? FS_REPLACE : a.wear >= 80 ? FS_SOON : FS_OK;

  a.avgLpd = a.days >= 1 ? a.liters / a.days : 0;
  float byVolume = a.avgLpd > 0.01f ? a.litersLeft / a.avgLpd : 1e6f;
  a.forecastDays = a.fstate == FS_REPLACE ? 0 : (int)min(a.daysLeft, byVolume);

  a.sensorFault = C.lastFlags & 1;
  a.tempWarn = C.lastTdsEpoch && C.lastTemp > TEMP_WARN_C;

  if (a.tds <= 0) a.wclass = WC_UNKNOWN;
  else if (a.tds <= S.tdsT1) a.wclass = WC_EXCELLENT;
  else if (a.tds <= S.tdsT2) a.wclass = WC_GOOD;
  else if (a.tds <= S.tdsT3) a.wclass = WC_FAIR;
  else a.wclass = WC_BAD;

  a.waterOk = a.wclass >= WC_EXCELLENT && a.wclass <= WC_FAIR && a.fstate != FS_REPLACE && !a.tempWarn;
  return a;
}
