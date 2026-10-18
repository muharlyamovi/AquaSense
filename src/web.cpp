#include "web.h"
#include "config.h"
#include "storage.h"
#include "quality.h"
#include "net.h"
#include "app.h"
#include "web_ui.h"
#include "ota.h"
#include "cloud.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

static WebServer server(80);
static DNSServer dns;
static bool running = false;
static bool apMode = false;
static bool sleepRequested = false;
static uint32_t lastActivity = 0;
static uint32_t timeoutMs = SERVICE_TIMEOUT_MS;

enum WifiTest : uint8_t { WT_IDLE = 0, WT_CONNECTING, WT_OK, WT_FAIL };
static WifiTest wifiTest = WT_IDLE;
static uint32_t wifiTestStart = 0;
static uint32_t wifiOkAt = 0;
static char pendSsid[33], pendPass[65];
static bool flowCalOn = false;
static uint32_t flowCalStart = 0;
static uint32_t flowCalEdgeStart = 0;

static float r1(float v) { return roundf(v * 10.0f) / 10.0f; }

static void sendJson(JsonDocument &d) {
  String s;
  serializeJson(d, s);
  server.send(200, "application/json", s);
}

static void sendOk(bool ok = true, const char *err = nullptr) {
  JsonDocument d;
  d["ok"] = ok;
  if (err) d["err"] = err;
  sendJson(d);
}

static void route(const char *path, HTTPMethod method, void (*fn)(), bool touch = true) {
  server.on(path, method, [fn, touch]() {
    if (touch) lastActivity = millis();
    fn();
  });
}

static void fillContacts(JsonArray arr) {
  ContactLog l;
  storageLoadContacts(l);
  for (uint8_t i = 0; i < l.count; i++) {
    const ContactEvent &e = l.e[(l.head + CONTACT_LOG_N - 1 - i) % CONTACT_LOG_N];
    JsonObject o = arr.add<JsonObject>();
    o["e"] = e.epoch;
    o["w"] = e.why;
  }
}

// ---------------- Статус ----------------

static void fillStatus(JsonDocument &d) {
  flowCollect();
  Assessment a = assess();
  uint32_t now = nowEpoch();
  d["name"] = S.devName;
  d["id"] = deviceId();
  d["fw"] = FW_VERSION;
  d["autoOta"] = S.autoOta != 0;
  d["otaHours"] = S.otaHours ? S.otaHours : 24;
  d["otaAge"] = C.lastOtaCheckEpoch ? (int32_t)(now - C.lastOtaCheckEpoch) : -1;
  d["cloudSet"] = S.ghToken[0] != 0;
  d["cloudAge"] = C.lastCloudEpoch ? (int32_t)(now - C.lastCloudEpoch) : -1;
  d["ghRepo"] = S.ghRepo;
  d["mode"] = apMode ? "ap" : "sta";
  d["ip"] = apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  d["ssid"] = S.ssid;
  d["configured"] = S.ssid[0] != 0;
  d["rssi"] = (!apMode && WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
  d["timeValid"] = timeValid();
  d["now"] = now;
  int32_t left = ((int32_t)timeoutMs - (int32_t)(millis() - lastActivity)) / 1000;
  d["svcLeft"] = left > 0 ? left : 0;
  d["btn"] = digitalRead(PIN_BUTTON) == LOW;
  fillContacts(d["log"].to<JsonArray>());

  d["tds"] = r1(C.lastTds);
  d["temp"] = r1(C.lastTemp);
  d["ema"] = r1(C.tdsEma);
  d["base"] = r1(C.baselineTds);
  d["tdsAge"] = C.lastTdsEpoch ? (int32_t)(now - C.lastTdsEpoch) : -1;
  d["wclass"] = (int)a.wclass;
  d["waterOk"] = a.waterOk;
  d["sensorFault"] = a.sensorFault;
  d["tempWarn"] = a.tempWarn;

  d["liters"] = r1(a.liters);
  d["litersLeft"] = r1(a.litersLeft);
  d["cl"] = S.cartridgeLiters;
  d["days"] = r1(a.days);
  d["daysLeft"] = r1(a.daysLeft);
  d["wear"] = r1(a.wear);
  d["wearVol"] = r1(a.wearVol);
  d["wearTime"] = r1(a.wearTime);
  d["wearTds"] = r1(a.wearTds);
  d["fstate"] = (int)a.fstate;
  d["reason"] = a.reason;
  d["t1"] = S.tdsT1;
  d["t2"] = S.tdsT2;
  d["t3"] = S.tdsT3;
  d["forecast"] = a.forecastDays;
  d["avgLpd"] = r1(a.avgLpd);
  d["install"] = C.installEpoch;

  d["battMv"] = C.battMv;
  d["battPct"] = C.battPct;
  d["leak"] = C.leakActive != 0;
  d["leakEn"] = LEAK_SENSOR_ENABLED != 0;
  d["total"] = r1(C.totalPulses * S.litersPerPulse);
  d["today"] = r1(litersToday());
  d["shop"] = S.shopUrl;
  d["model"] = S.model;
}

static void handleStatus() {
  JsonDocument d;
  fillStatus(d);
  sendJson(d);
}

// ---------------- Настройки ----------------

static void handleSettingsGet() {
  JsonDocument d;
  d["name"] = S.devName;
  d["model"] = S.model;
  d["shop"] = S.shopUrl;
  d["tgc"] = S.tgChat;
  d["tgSet"] = S.tgToken[0] != 0;
  d["lpp"] = S.litersPerPulse;
  d["cl"] = S.cartridgeLiters;
  d["cm"] = S.cartridgeMonths;
  d["ftype"] = S.filterType;
  d["t1"] = S.tdsT1;
  d["t2"] = S.tdsT2;
  d["t3"] = S.tdsT3;
  d["rise"] = S.tdsRise;
  d["k"] = S.tdsK;
  d["ota"] = S.otaUrl;
  d["autoOta"] = S.autoOta ? 1 : 0;
  d["otaHours"] = S.otaHours ? S.otaHours : 24;
  d["ghrepo"] = S.ghRepo;
  d["ghSet"] = S.ghToken[0] != 0;
  sendJson(d);
}

static void argStr(const char *key, char *dst, size_t n) {
  if (server.hasArg(key)) strlcpy(dst, server.arg(key).c_str(), n);
}

static void handleSettingsPost() {
  argStr("name", S.devName, sizeof(S.devName));
  argStr("model", S.model, sizeof(S.model));
  argStr("shop", S.shopUrl, sizeof(S.shopUrl));
  argStr("tgc", S.tgChat, sizeof(S.tgChat));
  if (server.arg("tgt").length() > 0) strlcpy(S.tgToken, server.arg("tgt").c_str(), sizeof(S.tgToken));
  if (server.hasArg("tgclear")) S.tgToken[0] = 0;

  if (server.hasArg("lpp")) { float v = server.arg("lpp").toFloat(); if (v > 0 && v <= 1000) S.litersPerPulse = v; }
  if (server.hasArg("cl")) { long v = server.arg("cl").toInt(); if (v > 0) S.cartridgeLiters = v; }
  if (server.hasArg("cm")) { long v = server.arg("cm").toInt(); if (v > 0 && v < 120) S.cartridgeMonths = v; }
  if (server.hasArg("ftype")) S.filterType = server.arg("ftype").toInt() ? FILTER_CARBON : FILTER_RO;
  if (server.hasArg("t1")) S.tdsT1 = server.arg("t1").toInt();
  if (server.hasArg("t2")) S.tdsT2 = server.arg("t2").toInt();
  if (server.hasArg("t3")) S.tdsT3 = server.arg("t3").toInt();
  if (server.hasArg("rise")) S.tdsRise = max(0.0f, server.arg("rise").toFloat());
  argStr("ota", S.otaUrl, sizeof(S.otaUrl));
  if (server.hasArg("autoOta")) S.autoOta = server.arg("autoOta").toInt() ? 1 : 0;
  if (server.hasArg("otaHours")) {
    long h = server.arg("otaHours").toInt();
    if (h >= 6 && h <= 168) S.otaHours = (uint16_t)h;
  }
  argStr("ghrepo", S.ghRepo, sizeof(S.ghRepo));
  if (server.arg("ght").length() > 0) strlcpy(S.ghToken, server.arg("ght").c_str(), sizeof(S.ghToken));
  if (server.hasArg("ghclear")) S.ghToken[0] = 0;
  storageSaveSettings();
  handleSettingsGet();
}

// ---------------- Действия ----------------

static void handleTime() {
  uint32_t e = strtoul(server.arg("epoch").c_str(), nullptr, 10);
  if (e > TIME_VALID_MIN) applyTime(e);
  sendOk();
}

static void handleMeasure() {
  measureWater();
  storageSaveCounters();
  handleStatus();
}

static void handleReplace() {
  resetCartridge();
  handleStatus();
}

static uint32_t flowCalPulses() {
  flowCollect();
  return C.totalPulses >= flowCalStart ? C.totalPulses - flowCalStart : 0;
}

static void sendFlowCal(bool ok = true, const char *err = nullptr) {
  JsonDocument d;
  d["ok"] = ok;
  if (err) d["err"] = err;
  d["on"] = flowCalOn;
  d["pulses"] = flowCalOn ? flowCalPulses() : 0;
  uint32_t edges = flowEdgeCount();
  d["edges"] = flowCalOn && edges >= flowCalEdgeStart ? edges - flowCalEdgeStart : 0;
  d["low"] = digitalRead(PIN_FLOW) == LOW;
  d["lpp"] = S.litersPerPulse;
  sendJson(d);
}

static void handleFlowCalGet() { sendFlowCal(); }

static void handleFlowCalPost() {
  String act = server.arg("action");
  if (act == "start") {
    flowCollect();
    flowCalStart = C.totalPulses;
    flowCalEdgeStart = flowEdgeCount();
    flowCalOn = true;
    timeoutMs = SETUP_TIMEOUT_MS;
    lastActivity = millis();
    return sendFlowCal();
  }
  if (act == "cancel") {
    flowCalOn = false;
    timeoutMs = SERVICE_TIMEOUT_MS;
    return sendFlowCal();
  }
  if (act == "stop") {
    float liters = server.arg("liters").toFloat();
    uint32_t n = flowCalPulses();
    flowCalOn = false;
    timeoutMs = SERVICE_TIMEOUT_MS;
    if (liters < 0.1f) return sendFlowCal(false, "укажите, сколько литров налили");
    if (n < 1) return sendFlowCal(false, "импульсов не было — налейте больше или проверьте геркон");
    float lpp = liters / (float)n;
    if (lpp < 0.001f || lpp > 1000.f) return sendFlowCal(false, "нереальное значение, проверьте объём");
    S.litersPerPulse = lpp;
    storageSaveSettings();
    JsonDocument d;
    d["ok"] = true;
    d["on"] = false;
    d["pulses"] = n;
    d["liters"] = liters;
    d["lpp"] = lpp;
    sendJson(d);
    return;
  }
  sendFlowCal(false, "неизвестное действие");
}

static void handleCalibrate() {
  float ref = server.arg("ref").toFloat();
  if (ref < 10) return sendOk(false, "укажите TDS раствора");
  WaterReading r;
  if (!waterMeasure(r) || r.tdsRaw < 1) return sendOk(false, "датчик не в растворе");
  S.tdsK = constrain(ref / r.tdsRaw, 0.3f, 3.0f);
  storageSaveSettings();
  JsonDocument d;
  d["ok"] = true;
  d["k"] = S.tdsK;
  d["temp"] = r.tempC;
  sendJson(d);
}

static void handleHistory() {
  History h;
  storageLoadHistory(h);
  JsonDocument d;
  JsonArray arr = d["days"].to<JsonArray>();
  for (uint8_t i = 0; i < h.count; i++) {
    const DayRecord &r = h.d[(h.head + HISTORY_DAYS - h.count + i) % HISTORY_DAYS];
    JsonObject o = arr.add<JsonObject>();
    o["e"] = r.epoch;
    o["l"] = r.liters10 / 10.0f;
    o["t"] = r.tds;
    o["b"] = r.batt;
  }
  flowCollect();
  d["today"] = r1(litersToday());
  d["todayTds"] = C.dayTdsN ? (int)(C.dayTdsSum / C.dayTdsN) : 0;
  d["t1"] = S.tdsT1;
  d["t2"] = S.tdsT2;
  d["t3"] = S.tdsT3;
  fillContacts(d["log"].to<JsonArray>());
  sendJson(d);
}

static String scanJson;

static void doScan() {
  int n = WiFi.scanNetworks();
  JsonDocument d;
  JsonArray arr = d.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    bool dup = false;
    for (JsonObject o : arr) if (ssid == o["ssid"].as<const char *>()) { dup = true; break; }
    if (dup) continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = ssid;
    o["rssi"] = WiFi.RSSI(i);
    o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  scanJson = "";
  serializeJson(d, scanJson);
}

static void handleScan() {
  if (server.hasArg("refresh") || !scanJson.length()) doScan();
  server.send(200, "application/json", scanJson);
}

static void handleWifi() {
  String ssid = server.arg("ssid");
  if (!ssid.length()) return sendOk(false, "пустое имя сети");
  if (!apMode) {
    // уже работаем в домашней сети — переподключение оборвало бы страницу, просто сохраняем
    strlcpy(S.ssid, ssid.c_str(), sizeof(S.ssid));
    strlcpy(S.pass, server.arg("pass").c_str(), sizeof(S.pass));
    storageSaveSettings();
    JsonDocument d;
    d["ok"] = true;
    d["saved"] = true;
    return sendJson(d);
  }
  strlcpy(pendSsid, ssid.c_str(), sizeof(pendSsid));
  strlcpy(pendPass, server.arg("pass").c_str(), sizeof(pendPass));
  WiFi.disconnect(false);
  WiFi.begin(pendSsid, pendPass);
  wifiTest = WT_CONNECTING;
  wifiTestStart = millis();
  sendOk();
}

static void handleWifiStatus() {
  JsonDocument d;
  d["state"] = (int)wifiTest;
  if (wifiTest == WT_OK) d["ip"] = WiFi.localIP().toString();
  sendJson(d);
}

static void handleTestTg() {
  if (apMode || WiFi.status() != WL_CONNECTED) return sendOk(false, "нет интернета в режиме точки доступа");
  bool ok = netTelegram(String(S.devName) + ": тестовое сообщение. Ссылка магазина: " + shopLink("buy"));
  sendOk(ok, ok ? nullptr : "проверьте token и chat id");
}

static void handleSleep() {
  sleepRequested = true;
  sendOk();
}

static void handleReset() {
  sendOk();
  delay(300);
  appFactoryReset();
}

static void handleCloudPush() {
  if (apMode && WiFi.status() != WL_CONNECTED) return sendOk(false, "нужен домашний Wi-Fi с интернетом");
  if (!cloudConfigured()) return sendOk(false, "задайте GitHub token");
  bool ok = cloudPush();
  sendOk(ok, ok ? nullptr : "GitHub не принял данные — проверьте token и доступ к репозиторию");
}

static void handleOtaCheck() {
  if (apMode && WiFi.status() != WL_CONNECTED) return sendOk(false, "нужен домашний Wi-Fi с интернетом");
  OtaInfo info;
  if (!otaFetch(info)) return sendOk(false, info.err);
  JsonDocument d;
  d["ok"] = true;
  d["current"] = FW_VERSION;
  d["latest"] = info.version;
  d["notes"] = info.notes;
  d["newer"] = info.newer;
  sendJson(d);
}

static void handleOtaUpdate() {
  if (apMode && WiFi.status() != WL_CONNECTED) return sendOk(false, "нужен домашний Wi-Fi с интернетом");
  if (C.battMv && C.battMv < BATT_LOW_MV) return sendOk(false, "зарядите батарею выше 3.45 В");
  OtaInfo info;
  if (!otaFetch(info)) return sendOk(false, info.err);
  if (!info.newer) return sendOk(false, "уже стоит последняя версия");
  JsonDocument d;
  d["ok"] = true;
  d["latest"] = info.version;
  sendJson(d);
  delay(500);
  otaApply(&info);
}

static void handleRoot() {
  if (apMode && !S.ssid[0]) server.send_P(200, "text/html", SETUP_HTML);
  else server.send_P(200, "text/html", INDEX_HTML);
}

static void handleSetup() {
  server.send_P(200, "text/html", SETUP_HTML);
}

static void handleCaptive() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

static void handleNotFound() {
  if (apMode) return handleCaptive();
  server.send(404, "text/plain", "Not found");
}

// ---------------- Жизненный цикл ----------------

void webStart(bool forceAp) {
  WiFi.persistent(false);
  apMode = forceAp || !S.ssid[0];
  if (!apMode && !netConnectSta(WIFI_CONNECT_TIMEOUT_MS)) apMode = true;

  if (apMode) {
    netOff();
    WiFi.mode(WIFI_STA);
    doScan();
    WiFi.mode(WIFI_AP_STA);
    String apName = "AquaSense-" + deviceId().substring(2);
    WiFi.softAP(apName.c_str(), AP_PASS[0] ? AP_PASS : nullptr);
    delay(100);
    dns.start(53, "*", WiFi.softAPIP());
    LOG("AP %s, http://%s\n", apName.c_str(), WiFi.softAPIP().toString().c_str());
  } else {
    WiFi.setSleep(true);
  }
  MDNS.begin(MDNS_NAME);
  MDNS.addService("http", "tcp", 80);

  route("/", HTTP_GET, handleRoot);
  route("/setup", HTTP_GET, handleSetup);
  route("/api/status", HTTP_GET, handleStatus, false);
  route("/api/settings", HTTP_GET, handleSettingsGet);
  route("/api/settings", HTTP_POST, handleSettingsPost);
  route("/api/time", HTTP_POST, handleTime);
  route("/api/measure", HTTP_POST, handleMeasure);
  route("/api/replace", HTTP_POST, handleReplace);
  route("/api/calibrate", HTTP_POST, handleCalibrate);
  route("/api/flowcal", HTTP_GET, handleFlowCalGet, false);
  route("/api/flowcal", HTTP_POST, handleFlowCalPost);
  route("/api/history", HTTP_GET, handleHistory, false);
  route("/api/scan", HTTP_GET, handleScan);
  route("/api/wifi", HTTP_POST, handleWifi);
  route("/api/wifistatus", HTTP_GET, handleWifiStatus, false);
  route("/api/testtg", HTTP_POST, handleTestTg);
  route("/api/sleep", HTTP_POST, handleSleep);
  route("/api/reset", HTTP_POST, handleReset);
  route("/api/ota", HTTP_GET, handleOtaCheck);
  route("/api/ota", HTTP_POST, handleOtaUpdate);
  route("/api/cloud", HTTP_POST, handleCloudPush);
  for (const char *p : {"/generate_204", "/gen_204", "/hotspot-detect.html", "/connecttest.txt", "/ncsi.txt", "/fwlink"})
    route(p, HTTP_ANY, handleCaptive);
  server.onNotFound([]() { lastActivity = millis(); handleNotFound(); });
  server.begin();

  running = true;
  sleepRequested = false;
  wifiTest = WT_IDLE;
  flowCalOn = false;
  timeoutMs = (apMode && !S.ssid[0]) ? SETUP_TIMEOUT_MS : SERVICE_TIMEOUT_MS;
  lastActivity = millis();
}

bool webLoop() {
  if (apMode) dns.processNextRequest();
  server.handleClient();

  if (wifiTest == WT_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      strlcpy(S.ssid, pendSsid, sizeof(S.ssid));
      strlcpy(S.pass, pendPass, sizeof(S.pass));
      storageSaveSettings();
      wifiTest = WT_OK;
      wifiOkAt = millis();
      timeoutMs = SERVICE_TIMEOUT_MS;
      LOG("wifi saved, ip %s\n", WiFi.localIP().toString().c_str());
    } else if (millis() - wifiTestStart > 20000) {
      WiFi.disconnect(false);
      wifiTest = WT_FAIL;
    }
  }

  // гасим точку доступа, чтобы телефон сам вернулся в домашнюю сеть; сервер продолжает работать там
  if (wifiTest == WT_OK && apMode && millis() - wifiOkAt > AP_OFF_AFTER_OK_MS) {
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);
    apMode = false;
    MDNS.end();
    MDNS.begin(MDNS_NAME);
    MDNS.addService("http", "tcp", 80);
    lastActivity = millis();
    LOG("AP off, staying on http://%s\n", WiFi.localIP().toString().c_str());
  }

  if (flowCalOn) lastActivity = millis();
  if (sleepRequested) return false;
  return millis() - lastActivity < timeoutMs;
}

void webStop() {
  server.stop();
  if (apMode) dns.stop();
  MDNS.end();
  netOff();
  running = false;
}

bool webRunning() { return running; }
bool webIsAp() { return apMode; }
bool webFlowCalActive() { return flowCalOn; }
void webTouch() { lastActivity = millis(); }
