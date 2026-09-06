#include "stepper.h"
#include <AccelStepper.h>
#include "debug.h"

// Motor 28BYJ-48
// Driver ULN2003 pin sequence on my board: IN1=15, IN2=2, IN3=5, IN4=18
// Change this to match the number of steps per revolution of your motor
constexpr uint8_t STEPPER_PIN_1 = 15; // IN1
constexpr uint8_t STEPPER_PIN_2 = 2;  // IN2
constexpr uint8_t STEPPER_PIN_3 = 5;  // IN3
constexpr uint8_t STEPPER_PIN_4 = 18; // IN4
static constexpr uint16_t STEPPER_SPEED = 250; // steps per second
static constexpr unsigned long IR_HOLD_TIMEOUT_MS = 250;

static AccelStepper stepper(AccelStepper::FULL4WIRE, STEPPER_PIN_1, STEPPER_PIN_2, STEPPER_PIN_3, STEPPER_PIN_4);
static unsigned long lastStepperCommand = 0;
static StepperState currentStepperState = StepperState::STEPPER_STOP;

void changeStepperState(StepperState state) {
    currentStepperState = state;
    lastStepperCommand = millis();

    switch (currentStepperState) {
        case StepperState::STEPPER_UP:
            stepper.setSpeed(STEPPER_SPEED);
            break;
        case StepperState::STEPPER_DOWN:
            stepper.setSpeed(-STEPPER_SPEED);
            break;
        case StepperState::STEPPER_STOP:
            stepper.setSpeed(0);
            break;
    }
}

void stepperLoop() {
    if (currentStepperState != StepperState::STEPPER_STOP
        && millis() - lastStepperCommand > IR_HOLD_TIMEOUT_MS) {
        changeStepperState(StepperState::STEPPER_STOP);
    }

    if (currentStepperState != StepperState::STEPPER_STOP) {
        stepper.runSpeed();
    }
}

void stepperDetach() {
    changeStepperState(StepperState::STEPPER_STOP);
    stepper.disableOutputs();
}

