#include <Arduino.h>

// Debounce configuration
const uint8_t buttonPin = 2;
const unsigned long debounceDelay = 50;  // milliseconds

// State tracking
bool buttonState = false;
bool lastStableState = false;
unsigned long lastDebounceTime = 0;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);  // Use pull-up for active-low button
  Serial.begin(115200);
}

void loop() {
  bool rawState = digitalRead(buttonPin);

  // If state changed, reset debounce timer
  if (rawState != lastStableState) {
    lastDebounceTime = millis();
    lastStableState = rawState;
  }
  
  // If stable for debounceDelay, update buttonState
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (rawState != buttonState) {
      buttonState = rawState;
      onButtonChange(buttonState);
    }
  }
}

void onButtonChange(bool state) {
  if (!state) {
    Serial.println("Button pressed");
    // Trigger your event here (e.g., toggle servo, send IR)
  } else {
    Serial.println("Button released");
  }
}
