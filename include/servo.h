#pragma once
#include <Arduino.h>

enum class ServoState : uint8_t {
    SERVO_STOP,
    SERVO_UP,
    SERVO_DOWN,
};

void changeServoState(ServoState state);
void servoLoop();
void servoDetach();