#pragma once
#include <Arduino.h>

bool netConnectSta(uint32_t timeoutMs);
void netOff();
bool netSyncTime();
bool netTelegram(const String &text);
String netUrlEncode(const String &s);
