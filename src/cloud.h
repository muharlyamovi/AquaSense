#pragma once
#include <Arduino.h>

bool cloudConfigured();
bool cloudPush();     // пишет data/latest.json в приватный репозиторий
void cloudMaybe();    // если подошёл интервал
