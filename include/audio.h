#pragma once
#include <Arduino.h>

enum class RadioStatus : uint8_t {
    IDLE,
    CONNECTING_WIFI,
    NO_WIFI,
    CONNECTING,
    PLAYING,
    STATION_FAILED,
};

extern const char* currentRadioStationName;

void i2sInSetup();
void i2sOutSetup();
void detachAudio();

void a2dpStart();
bool a2dpIsConnected();

void radioStart();
RadioStatus radioGetStatus();
void copierInOutCopy();

void audioNext();
void audioPrevious();
void audioPlayPause();
