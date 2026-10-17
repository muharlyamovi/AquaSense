#pragma once
#include <Arduino.h>

// Функции верхнего уровня из main.cpp, которыми пользуется веб-интерфейс
uint32_t flowCollect();
uint32_t flowEdgeCount();
void notifyQueue(const String &msg);
void notifyFlush();
void readBattery();
String shopLink(const char *action);
[[noreturn]] void appFactoryReset();
