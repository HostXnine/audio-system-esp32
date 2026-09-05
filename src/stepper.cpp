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

static AccelStepper stepper(AccelStepper::FULL4WIRE, STEPPER_PIN_1, STEPPER_PIN_2, STEPPER_PIN_3, STEPPER_PIN_4);


void stepperLoop() {
    if (stepper.distanceToGo() == 0) {
        stepper.moveTo(1);
        stepper.setMaxSpeed(200);
        stepper.setAcceleration(100);
    }
    stepper.run();
}