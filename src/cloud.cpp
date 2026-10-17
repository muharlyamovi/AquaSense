#include "cloud.h"
#include "config.h"
#include "storage.h"
#include "quality.h"
#include "net.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

static String apiUrl(const char *path) {
  return String("https://api.github.com/repos/") + S.ghRepo + "/contents/" + path;
}

static void ghHeaders(HTTPClient &http) {
  http.addHeader("Authorization", String("Bearer ") + S.ghToken);
  http.addHeader("User-Agent", "AquaSense");
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("X-GitHub-Api-Version", "2022-11-28");
}

bool cloudConfigured() { return S.ghToken[0] && S.ghRepo[0]; }

static String statusJson() {
  Assessment a = assess();
  History h;
  storageLoadHistory(h);
  JsonDocument d;
  d["id"] = deviceId();
  d["name"] = S.devName;
  d["fw"] = FW_VERSION;
  d["now"] = nowEpoch();
  d["tds"] = C.lastTds;
  d["temp"] = C.lastTemp;
  d["ema"] = C.tdsEma;
  d["base"] = C.baselineTds;
  d["wclass"] = (int)a.wclass;
  d["waterOk"] = a.waterOk;
  d["sensorFault"] = a.sensorFault;
  d["tempWarn"] = a.tempWarn;
  d["liters"] = a.liters;
  d["litersLeft"] = a.litersLeft;
  d["cl"] = S.cartridgeLiters;
  d["days"] = a.days;
  d["wear"] = a.wear;
  d["wearVol"] = a.wearVol;
  d["wearTime"] = a.wearTime;
  d["wearTds"] = a.wearTds;
  d["fstate"] = (int)a.fstate;
  d["reason"] = a.reason;
  d["t1"] = S.tdsT1;
  d["t2"] = S.tdsT2;
  d["t3"] = S.tdsT3;
  d["forecast"] = a.forecastDays;
  d["avgLpd"] = a.avgLpd;
  d["install"] = C.installEpoch;
  d["battMv"] = C.battMv;
  d["battPct"] = C.battPct;
  d["leak"] = C.leakActive != 0;
  d["total"] = C.totalPulses * S.litersPerPulse;
  d["today"] = litersToday();
  d["model"] = S.model;
  d["shop"] = S.shopUrl;
  JsonArray arr = d["hist"].to<JsonArray>();
  for (uint8_t i = 0; i < h.count; i++) {
    const DayRecord &r = h.d[(h.head + HISTORY_DAYS - h.count + i) % HISTORY_DAYS];
    JsonObject o = arr.add<JsonObject>();
    o["e"] = r.epoch;
    o["l"] = r.liters10 / 10.0f;
    o["t"] = r.tds;
    o["b"] = r.batt;
  }
  String s;
  serializeJson(d, s);
  return s;
}

static bool getFileSha(const char *path, String &sha) {
  sha = "";
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(12000);
  if (!http.begin(client, apiUrl(path))) return false;
  ghHeaders(http);
  int code = http.GET();
  if (code == 404) { http.end(); return true; }
  if (code != 200) {
    LOG("cloud GET %s -> %d\n", path, code);
    http.end();
    return false;
  }
  JsonDocument d;
  deserializeJson(d, http.getString());
  http.end();
  sha = d["sha"] | "";
  return true;
}

bool cloudPush() {
  if (!cloudConfigured() || WiFi.status() != WL_CONNECTED) return false;
  String body = statusJson();
  String sha;
  if (!getFileSha("data/latest.json", sha)) return false;

  JsonDocument req;
  req["message"] = String("status ") + nowEpoch();
  req["content"] = base64::encode(body);
  if (sha.length()) req["sha"] = sha;

  String payload;
  serializeJson(req, payload);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(client, apiUrl("data/latest.json"))) return false;
  ghHeaders(http);
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(payload);
  http.end();
  bool ok = code == 200 || code == 201;
  LOG("cloud PUT -> %d\n", code);
  if (ok) {
    C.lastCloudEpoch = nowEpoch();
    storageSaveCounters();
  }
  return ok;
}

void cloudMaybe() {
  if (!cloudConfigured() || !S.ssid[0]) return;
  uint32_t now = nowEpoch();
  if (C.lastCloudEpoch && now - C.lastCloudEpoch < CLOUD_INTERVAL_S) return;

  bool wasConnected = WiFi.status() == WL_CONNECTED;
  if (!wasConnected) {
    setCpuFrequencyMhz(160);
    if (!netConnectSta(WIFI_CONNECT_TIMEOUT_MS)) {
      setCpuFrequencyMhz(80);
      return;
    }
  }
  cloudPush();
  if (!wasConnected) {
    netOff();
    setCpuFrequencyMhz(80);
  }
}
