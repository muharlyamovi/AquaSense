#pragma once
#include <Arduino.h>

struct WaterReading {
  bool ok;        // датчик в воде, значение годное
  bool dry;
  bool ntcOk;
  int tdsMv;
  int ntcMv;
  float tempC;
  float tdsRaw;   // без калибровочного коэффициента
  float tds;
};

struct PowerCheck {
  int tdsOff, ntcOff, tdsOn, ntcOn;
  bool switched;   // питание реально снимается: при «выкл» АЦП почти ноль
};

void hwInitPins();
void hwPrepareSleep();
void hwPowerCheck(PowerCheck &p);

void sensorsPower(bool on);
bool waterMeasure(WaterReading &r);
float tdsFromMv(float mv, float tempC, float k);

bool gaugeRead(uint16_t &mv);
void gaugeSleep();
uint8_t battPctFromMv(uint16_t mv);

void ledSet(bool on);
void ledBlink(uint8_t n, uint16_t onMs, uint16_t offMs);
void beep(uint8_t n, uint16_t ms);
