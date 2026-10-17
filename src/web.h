#pragma once
#include <Arduino.h>

void webStart(bool forceAp);
bool webLoop();     // false — пора спать
void webStop();
bool webRunning();
bool webIsAp();
