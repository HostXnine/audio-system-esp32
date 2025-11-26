#pragma once

extern const char* currentRadioStationName;

void i2sInSetup();
void i2sOutSetup();
void detachAudio();

void a2dpStart();
bool a2dpIsConnected();

void playerBegin();
void copierInOutCopy();
void playerCopy();

void audioNext();
void audioPrevious();
void audioPlayPause();
