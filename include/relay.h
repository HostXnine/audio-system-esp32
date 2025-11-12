#pragma once
#include <Arduino.h>

constexpr uint8_t ON_OFF_5V_PIN = 13;
constexpr uint8_t ON_OFF_AC_PIN = 26;
constexpr uint8_t AUX_LEFT_PIN = 25;
constexpr uint8_t AUX_RIGHT_PIN = 33;

void relayPowerSetup();
void relayAuxSetup();