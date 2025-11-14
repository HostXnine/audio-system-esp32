#pragma once
#include <Arduino.h>

extern uint16_t postitionRtc;
extern uint16_t position; // servo position stored in ms

//Servo is still at 1490 - 1540 if you use a 360 (SG90) servo. Speed min 900 - max 2100. Ideal stop pint is 1515 ms.
//I have set the forward and backward motion speed to 100 milliseconds difference from the stop point.

//Checks if servo is attached, if not it attaches it. Don't put servo attach in setup() because it makes it jitter.
void servoSetup();
void servoDetach();