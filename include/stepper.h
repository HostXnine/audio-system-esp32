#pragma once
#include <Arduino.h>

enum class StepperState : uint8_t {
    STEPPER_STOP,
    STEPPER_UP,
    STEPPER_DOWN,
};

void changeStepperState(StepperState state);
void stepperLoopRandom();
void stepperDetach();