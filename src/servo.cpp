#include "servo.h"
#include <ESP32Servo.h>
#include "debug.h"

//Servo is still at 1490 - 1540 if you use a 360 (SG90) servo. Speed min 900 - max 2100. Ideal stop pint is 1515 ms.
//I have set the forward and backward motion speed to 100 milliseconds difference from the stop point.
//Checks if servo is attached, if not it attaches it. Don't put servo attach in setup() because it makes it jitter.

constexpr uint8_t SERVO_PIN = 23; //recommended pins 2 (if no led),4,12-19,21-23,25-27,32-33
constexpr uint16_t STOP = 1515;
constexpr uint16_t UP = 1550;
constexpr uint16_t DOWN = 1480;

static bool debounceStart = false;
static unsigned long lastDebounceTime = 0;
static Servo MyServo;

static ServoState currentServoState = ServoState::SERVO_STOP;

void servoSetup() {
    debugln("servoSetup");
    if (!MyServo.attached()) { 
        MyServo.attach(SERVO_PIN, DOWN, UP);
    }
}

void servoState(ServoState state) {
    switch(state) {
        case ServoState::SERVO_STOP:
            MyServo.writeMicroseconds(STOP);
            break;
        case ServoState::SERVO_UP:
            MyServo.writeMicroseconds(UP);
            break;
        case ServoState::SERVO_DOWN:
            MyServo.writeMicroseconds(DOWN);
            break;
        default:
            break;
    }
}

void setServoState(ServoState state) {
    currentServoState = state;
    servoState(currentServoState);
}

void debounceServo(unsigned long debounceDelayServo = 10) {
    static bool initDebounce = false;
    if (!initDebounce) {
        initDebounce = true;
        lastDebounceTime = millis();
        debugln(" Debounce Servo start ");
    }
    if (((millis() - lastDebounceTime) > (debounceDelayServo)) && (initDebounce)) {
        initDebounce = false;
        debugln("Debounce Servo ends");
        debounceStart = false;
        setServoState(ServoState::SERVO_STOP);
    }
}

void changeServoState(ServoState state) {
    servoSetup();
    if (currentServoState != state) {
        setServoState(state);
        debounceStart = true;
    }
    if (currentServoState == state) {
        lastDebounceTime = millis(); //resets debounce timer
    }
}

void servoLoop() {
    if (debounceStart) {
        debounceServo();
    }
}

void servoDetach() {
    MyServo.detach();
}