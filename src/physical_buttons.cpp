#include "physical_buttons.h"
#include <Arduino.h>
#include "debug.h"

constexpr uint8_t BUTTON_A_PIN = 27;
constexpr uint8_t BUTTON_B_PIN = 14;

void physicalButtonsSetup() {
    debugln("Buttons setup");
    pinMode(BUTTON_A_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON_B_PIN, INPUT_PULLDOWN);
}