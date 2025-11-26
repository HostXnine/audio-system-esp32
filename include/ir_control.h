#pragma once
#include <Arduino.h>

enum class IrState : uint8_t {
    IR_RECEIVE_STATE,
    IR_DEBOUNCE_STATE,
    IR_RESET_STATE,
    STATE_IR_COUNT
};

extern IrState currentIrState;

extern unsigned long debounceDelayIr;

void irSetup();
void decodeNewRemote();
void irStateLoop(enum IrState);
void changeIrState();

