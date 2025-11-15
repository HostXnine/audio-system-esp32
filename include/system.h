#pragma once
#include <Arduino.h>

enum class SystemState : uint8_t {
    TV_STATE,
    BLUETOOTH_STATE,
    RADIO_STATE,
    AUX_STATE,
    OFF_STATE, //keep it always before the last item
    STATE_COUNT // keep it always last
};

extern enum SystemState currentSystemState;

void changeSystemState();
void changeSystemState();
void systemSetup();
void systemStateLoop(enum SystemState);
void changeOnOffSystemState();