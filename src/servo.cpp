#include "servo.h"
#include <ESP32Servo.h>

RTC_NOINIT_ATTR uint16_t postitionRtc = 0; // servo postition stored in ms
uint16_t position = SERVO_MIN; // servo position stored in ms
uint16_t step = 10; // Servo's one step movement in ms

Servo MyServo;

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