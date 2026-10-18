#pragma once
#include <Arduino.h>

void webStart(bool forceAp);
bool webLoop();     // false — пора спать
void webStop();
bool webRunning();
bool webIsAp();
bool webFlowCalActive();
void webTouch();   // сбросить таймер сна (кнопка GPIO5, действие в кабинете)
