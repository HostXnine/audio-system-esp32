#include <Arduino.h>
#include "display.h"
#include "relay.h"
#include "audio.h"
#include "physical_buttons.h"
#include "ir_control.h"
#include "system.h"
#include "menu.h"
#include "stepper.h"
#include "debug.h"

void setup() {
    //Serial.begin(115200);
    debugln("Setup started");
    relayPowerSetup();
    relayAuxSetup(); // must run before systemSetup(): AUX_STATE switches these pins HIGH
    displaySetup();
    irSetup();
    stepperSetup();
    systemSetup();
    //physicalButtonsSetup();
    debugln("Setup done");
}

void loop() {
    //decodeNewRemote(); //uncomment this for decoding a new remote
    irStateLoop(currentIrState);
    systemStateLoop(currentSystemState);
}