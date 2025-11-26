#pragma once

enum class IrState {
    IR_RECEIVE_STATE,
    IR_DEBOUNCE_STATE,
    IR_RESET_STATE,
    STATE_IR_COUNT
};

extern IrState currentIrState;

extern unsigned long debounceDelay;

void irSetup();
void decodeNewRemote();
void irStateLoop(enum IrState);
void changeIrState();

