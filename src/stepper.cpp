#include "stepper.h"
#include <AccelStepper.h>
#include "debug.h"

// Motor 28BYJ-48
// Driver ULN2003 pin sequence on my board: IN1=15, IN2=2, IN3=5, IN4=18
constexpr uint8_t STEPPER_PIN_1 = 15; // IN1
constexpr uint8_t STEPPER_PIN_2 = 2;  // IN2
constexpr uint8_t STEPPER_PIN_3 = 5;  // IN3
constexpr uint8_t STEPPER_PIN_4 = 18; // IN4
static constexpr float STEPPER_SPEED = 250.0f; // full steps per second (28BYJ-48 tops out around ~500)
// NEC remotes send a repeat frame every ~108 ms while a button is held.
// If no frame arrives within this time, the button was released -> stop.
static constexpr unsigned long IR_HOLD_TIMEOUT_MS = 200;

// IMPORTANT: AccelStepper FULL4WIRE with a 28BYJ-48 + ULN2003 needs the pins in
// the order IN1, IN3, IN2, IN4. With IN1, IN2, IN3, IN4 the coils are energized
// in the wrong sequence and the motor just buzzes in place.
static AccelStepper stepper(AccelStepper::FULL4WIRE, STEPPER_PIN_1, STEPPER_PIN_3, STEPPER_PIN_2, STEPPER_PIN_4);

// Written from the IR code (loop task), read by the stepper task.
static volatile StepperState requestedState = StepperState::STEPPER_STOP;
static volatile unsigned long lastStepperCommand = 0;
static TaskHandle_t stepperTaskHandle = nullptr;

void changeStepperState(StepperState state) {
    lastStepperCommand = millis();
    requestedState = state;
}

// Runs in its own FreeRTOS task, so stepping timing doesn't depend on how long
// the audio copy in loop() blocks, and the audio isn't disturbed by stepping.
static void stepperTask(void*) {
    StepperState activeState = StepperState::STEPPER_STOP;

    for (;;) {
        StepperState wanted = requestedState;

        if (wanted != StepperState::STEPPER_STOP
            && millis() - lastStepperCommand > IR_HOLD_TIMEOUT_MS) {
            wanted = StepperState::STEPPER_STOP;
            requestedState = StepperState::STEPPER_STOP;
        }

        if (wanted != activeState) {
            activeState = wanted;
            switch (activeState) {
                case StepperState::STEPPER_UP:
                    stepper.enableOutputs();
                    stepper.setSpeed(STEPPER_SPEED);
                    break;
                case StepperState::STEPPER_DOWN:
                    stepper.enableOutputs();
                    stepper.setSpeed(-STEPPER_SPEED);
                    break;
                case StepperState::STEPPER_STOP:
                    stepper.setSpeed(0);
                    // De-energize coils: the 28BYJ-48 gearbox holds position by itself,
                    // and this avoids heat and current draw (noise) while idle.
                    stepper.disableOutputs();
                    break;
            }
        }

        if (activeState != StepperState::STEPPER_STOP) {
            stepper.runSpeed();
        }

        vTaskDelay(1); // 1 ms tick; at 250 steps/s a step is due every 4 ms
    }
}

void stepperSetup() {
    debugln("Stepper setup");
    // AccelStepper's default max speed is 1 step/s and setSpeed() is clamped to it,
    // so without this the motor only twitches once per second.
    stepper.setMaxSpeed(STEPPER_SPEED);
    stepper.disableOutputs();
    if (stepperTaskHandle == nullptr) {
        // Core 1 (same as loop), priority 3 so it preempts loop() and the radio task
        // briefly for each step. Core 0 is left to WiFi / Bluetooth.
        xTaskCreatePinnedToCore(stepperTask, "stepper", 2048, nullptr, 3, &stepperTaskHandle, 1);
    }
}

void stepperDetach() {
    requestedState = StepperState::STEPPER_STOP;
    if (stepperTaskHandle != nullptr) {
        vTaskDelete(stepperTaskHandle);
        stepperTaskHandle = nullptr;
    }
    stepper.disableOutputs();
}
