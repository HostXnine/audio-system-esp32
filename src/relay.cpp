#include "relay.h"
#include <Arduino.h>
#include "debug.h"

constexpr uint8_t ON_OFF_5V_PIN = 13;
constexpr uint8_t ON_OFF_AC_PIN = 26;
constexpr uint8_t AUX_LEFT_PIN = 25;
constexpr uint8_t AUX_RIGHT_PIN = 33;

void relayPowerSetup() {
    debugln("Relays setup");
    pinMode(ON_OFF_5V_PIN, OUTPUT);
    digitalWrite(ON_OFF_5V_PIN, HIGH);
    pinMode(ON_OFF_AC_PIN, OUTPUT);
    digitalWrite(ON_OFF_AC_PIN, HIGH);
}
void relayAuxSetup() {
    debugln("Relay AUX setup");
    pinMode(AUX_LEFT_PIN, OUTPUT);
    digitalWrite(AUX_LEFT_PIN, LOW); //when LOW then it plays ADC IN
    pinMode(AUX_RIGHT_PIN, OUTPUT);
    digitalWrite(AUX_RIGHT_PIN, LOW);
}

void relayRestart() {
    debugln(" Relay digitalWrite ON_OFF_PIN");
    if (digitalRead(ON_OFF_5V_PIN) == LOW) {
        digitalWrite(ON_OFF_5V_PIN, HIGH);
    }
    if (digitalRead(ON_OFF_AC_PIN) == LOW) {
        digitalWrite(ON_OFF_AC_PIN, HIGH);
    }
    if (digitalRead(AUX_LEFT_PIN) == HIGH || digitalRead(AUX_RIGHT_PIN) == HIGH) {
        digitalWrite(AUX_LEFT_PIN, LOW);
        digitalWrite(AUX_RIGHT_PIN, LOW);
    }
}

void relayAuxState() {
    pinMode(AUX_LEFT_PIN, OUTPUT);
    pinMode(AUX_RIGHT_PIN, OUTPUT);
    if (digitalRead(AUX_LEFT_PIN) == LOW || digitalRead(AUX_RIGHT_PIN) == LOW) {
        digitalWrite(AUX_LEFT_PIN, HIGH);
        digitalWrite(AUX_RIGHT_PIN, HIGH);
    }
}

void relayOffState() {
    digitalWrite(AUX_LEFT_PIN, LOW);
    digitalWrite(AUX_RIGHT_PIN, LOW);
    digitalWrite(ON_OFF_5V_PIN, LOW);
    digitalWrite(ON_OFF_AC_PIN, LOW);
}