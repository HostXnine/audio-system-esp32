#include <Arduino.h>
#include "display.h"
#include "servo.h"
#include "relay.h"
#include "audio.h"
#include "physical_buttons.h"
#include "ir_control.h"
#include "system.h"
#include "menu.h"
#include "stepper.h"
#include "debug.h"

void setup() {
    Serial.begin(115200);
    debugln("Setup started");
    relayPowerSetup();
    displaySetup();
    irSetup();
    systemSetup();
    //physicalButtonsSetup();   
    debugln("Setup done");
}

void loop() {
    //decodeNewRemote(); //uncomment this for decoding a new remote
    irStateLoop(currentIrState);
    systemStateLoop(currentSystemState);
    stepperLoopRandom(); // only for testing
}g