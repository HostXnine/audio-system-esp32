#pragma once
#include <Arduino.h>

//Audio OUT (external DAC)
constexpr uint8_t OUT_I2S_BCK_PIN = 17; //aka SCK. Connect to (BCK)
constexpr uint8_t OUT_I2S_DATA_PIN = 16; //Connect to (DIN)
constexpr uint8_t OUT_I2S_WS_PIN = 4; //aka LRCLK. Connect to (LCK)
// SCK connect to GND

//Aduio IN (external SPDIF to I2S converer)
constexpr uint8_t IN_I2S_WS_PIN = 34; //aka LRCLK. Connect to (LRCK)
constexpr uint8_t IN_I2S_DATA_PIN = 35; // data out. Connect to (DATA)
constexpr uint8_t IN_I2S_BCK_PIN = 32; //aka SCK. Connect to (BCKL)
//constexpr uint8_t IN_I2S_MCK_PIN = 3;  //must be 0,1 or 3. NEVER USE 1 it's the TX pin. 3 is the RX pin and 0 is not exposed. The external DAC works without MCK connection.

extern uint8_t currentStation;

extern const char* urls[3];
extern const char* radioStationNames[3];

void i2sInSetup();
void i2sOutSetup();
void detachAudio();
void a2dpSinkStart();
void playerBegin();
void copierInOutCopy();
void playerCopy();