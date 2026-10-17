#include "net.h"
#include "config.h"
#include "storage.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

bool netConnectSta(uint32_t timeoutMs) {
  if (!S.ssid[0]) return false;
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(S.ssid, S.pass);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) delay(50);
  bool ok = WiFi.status() == WL_CONNECTED;
  LOG("wifi %s: %s\n", S.ssid, ok ? WiFi.localIP().toString().c_str() : "FAILED");
  return ok;
}

void netOff() {
  if (WiFi.getMode() == WIFI_OFF) return;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool netSyncTime() {
  bool wasValid = timeValid();
  uint32_t before = nowEpoch();
  uint32_t m0 = millis();
  C.lastNtpTryEpoch = before;
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  while (millis() - m0 < 6000 && nowEpoch() < TIME_VALID_MIN) delay(100);
  if (!timeValid()) return false;
  if (!wasValid) {
    uint32_t expected = before + (millis() - m0) / 1000;
    shiftStoredEpochs((int64_t)nowEpoch() - expected);
  }
  C.lastNtpEpoch = nowEpoch();
  LOG("NTP ok: %u\n", C.lastNtpEpoch);
  return true;
}

String netUrlEncode(const String &s) {
  static const char hex[] = "0123456789ABCDEF";
  String out;
  out.reserve(s.length() * 3);
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}

bool netTelegram(const String &text) {
  if (!S.tgToken[0] || !S.tgChat[0] || WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String("https://api.telegram.org/bot") + S.tgToken + "/sendMessage";
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int code = http.POST("chat_id=" + netUrlEncode(S.tgChat) + "&text=" + netUrlEncode(text));
  http.end();
  LOG("telegram -> %d\n", code);
  return code == 200;
}
