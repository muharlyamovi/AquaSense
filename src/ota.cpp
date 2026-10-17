#include "ota.h"
#include "config.h"
#include "storage.h"
#include "hw.h"
#include "net.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

static int verNum(const char *s) {
  int a = 0, b = 0, c = 0;
  sscanf(s ? s : "", "%d.%d.%d", &a, &b, &c);
  return (a << 16) | (b << 8) | c;
}

bool otaConfigured() { return S.otaUrl[0] != 0; }

static void setErr(OtaInfo &info, const char *e) {
  info.ok = false;
  strlcpy(info.err, e, sizeof(info.err));
}

bool otaFetch(OtaInfo &info) {
  memset(&info, 0, sizeof(info));
  if (!S.otaUrl[0]) { setErr(info, "не задан URL манифеста"); return false; }
  if (WiFi.status() != WL_CONNECTED) { setErr(info, "нет интернета"); return false; }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  if (!http.begin(client, S.otaUrl)) { setErr(info, "неверный URL"); return false; }
  int code = http.GET();
  if (code != 200) {
    snprintf(info.err, sizeof(info.err), "манифест HTTP %d", code);
    http.end();
    return false;
  }
  JsonDocument d;
  DeserializationError e = deserializeJson(d, http.getStream());
  http.end();
  if (e) { setErr(info, "манифест не JSON"); return false; }

  strlcpy(info.version, d["version"] | "", sizeof(info.version));
  strlcpy(info.url, d["url"] | "", sizeof(info.url));
  strlcpy(info.notes, d["notes"] | "", sizeof(info.notes));
  if (!info.version[0] || !info.url[0]) { setErr(info, "в манифесте нет version/url"); return false; }

  info.ok = true;
  info.newer = verNum(info.version) > verNum(FW_VERSION);
  C.lastOtaCheckEpoch = nowEpoch();
  LOG("ota: current %s, latest %s%s\n", FW_VERSION, info.version, info.newer ? " (newer)" : "");
  return true;
}

bool otaApply(const OtaInfo *given) {
  OtaInfo info;
  if (given) info = *given;
  else if (!otaFetch(info)) return false;
  if (!info.newer) {
    LOG("ota: already up to date\n");
    return false;
  }
  if (C.battMv && C.battMv < BATT_LOW_MV) {
    LOG("ota: battery too low\n");
    return false;
  }

  setCpuFrequencyMhz(160);
  WiFi.setSleep(false);
  ledSet(true);
  LOG("ota: downloading %s\n", info.url);

  WiFiClientSecure client;
  client.setInsecure();
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);
  httpUpdate.onProgress([](int cur, int total) {
    if (total > 0 && (cur % 50000 < 4096)) LOG("ota: %d / %d\n", cur, total);
    ledSet((cur / 32768) & 1);
  });

  t_httpUpdate_return r = httpUpdate.update(client, info.url);
  ledSet(false);
  setCpuFrequencyMhz(80);
  LOG("ota failed: %s\n", httpUpdate.getLastErrorString().c_str());
  (void)r;
  return false;
}

void otaMaybeAuto() {
  if (!S.autoOta || !S.otaUrl[0] || !S.ssid[0]) return;
  uint32_t now = nowEpoch();
  uint32_t interval = (uint32_t)constrain((int)S.otaHours, 6, 168) * 3600UL;
  if (C.lastOtaCheckEpoch && now - C.lastOtaCheckEpoch < interval) return;
  if (C.battMv && C.battMv < BATT_LOW_MV) return;

  bool wasConnected = WiFi.status() == WL_CONNECTED;
  if (!wasConnected) {
    setCpuFrequencyMhz(160);
    if (!netConnectSta(WIFI_CONNECT_TIMEOUT_MS)) {
      setCpuFrequencyMhz(80);
      return;
    }
  }

  OtaInfo info;
  if (otaFetch(info) && info.newer) {
    uint32_t packed = (uint32_t)verNum(info.version);
    if (C.lastOtaNotifyVer != packed) {
      C.lastOtaNotifyVer = packed;
      storageSaveCounters();
      netTelegram(String(S.devName) + ": ставлю прошивку " + info.version +
                  (info.notes[0] ? String(" — ") + info.notes : ""));
    }
    otaApply(&info);   // при успехе перезагрузится
  }

  if (!wasConnected) {
    netOff();
    setCpuFrequencyMhz(80);
  }
}
