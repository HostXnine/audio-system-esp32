#include "relay.h"

void relayPowerSetup() {
    Serial.println("Relays setup");
    pinMode(ON_OFF_5V_PIN, OUTPUT);
    digitalWrite(ON_OFF_5V_PIN, HIGH);
    pinMode(ON_OFF_AC_PIN, OUTPUT);
    digitalWrite(ON_OFF_AC_PIN, HIGH);
}
void relayAuxSetup() {
    Serial.println("AUX setup");
    pinMode(AUX_LEFT_PIN, OUTPUT);
    digitalWrite(AUX_LEFT_PIN, LOW); //when LOW then it plays ADC IN
    pinMode(AUX_RIGHT_PIN, OUTPUT);
    digitalWrite(AUX_RIGHT_PIN, LOW);
}