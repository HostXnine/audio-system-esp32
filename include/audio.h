#pragma once
#include <Arduino.h>

extern uint8_t currentStation;

extern const char* radioStationNames[3];

class BluetoothA2DPSink;
extern BluetoothA2DPSink a2dp_sink;

void i2sInSetup();
void i2sOutSetup();
void detachAudio();
void a2dpSinkStart();
void playerBegin();
void copierInOutCopy();
void playerCopy();