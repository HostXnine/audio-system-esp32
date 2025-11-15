#pragma once

extern const char* currentRadioStationName;

class BluetoothA2DPSink;
extern BluetoothA2DPSink a2dp_sink;

void i2sInSetup();
void i2sOutSetup();
void detachAudio();

void a2dpStart();
void playerBegin();
void copierInOutCopy();
void playerCopy();

void audioNext();
void audioPrevious();
void audioPlayPause();
