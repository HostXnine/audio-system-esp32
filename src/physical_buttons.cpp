#include "physical_buttons.h"

void physicalButtonsSetup() {
    Serial.println("Buttons setup");
    pinMode(BUTTON_A_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON_B_PIN, INPUT_PULLDOWN);
}