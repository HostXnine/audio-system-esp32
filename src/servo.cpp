#include "servo.h"
#include <ESP32Servo.h>

constexpr uint8_t SERVO_PIN = 23; //recommended pins 2 (if no led),4,12-19,21-23,25-27,32-33
constexpr uint16_t SERVO_MIN = 510;
constexpr uint16_t SERVO_MAX = 2510;
RTC_NOINIT_ATTR uint16_t postitionRtc; // servo postition stored in ms
uint16_t position = SERVO_MIN; // servo position stored in ms
uint16_t step = 10; // Servo's one step movement in ms

static Servo MyServo;

void isServoAttached() {
  Serial.println("isServoAttached");
  if (MyServo.attached() == false) { 
    MyServo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
    position = postitionRtc;
  }
}

void servoSetup() {
  if ((postitionRtc < SERVO_MIN-100) || (postitionRtc > SERVO_MAX+100)) {
    Serial.println("calibrating positionRTC setup");
    postitionRtc = SERVO_MIN+500;
  }
}

void servoDetach() {
  MyServo.detach();
}