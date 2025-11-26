#pragma once
#include <Arduino.h>

enum class Buttons : uint8_t {
    MENU_BUTTON = 5,
    POWER_BUTTON = 20,
    VOLUME_UP_BUTTON = 24,
    VOLUME_DOWN_BUTTON = 25,
    DEBUG_BUTTON = 240,
    NEXT_BUTTON = 3,
    PREVIOUS_BUTTON = 2,
    STOP_BUTTON = 1,
    PAUSE_BUTTON = 11,
    PLAY_BUTTON = 8,
    NONE_BUTTON = 0,
};

extern Buttons currentButton;

bool validButton(Buttons button);
void buttonInput(Buttons &button);