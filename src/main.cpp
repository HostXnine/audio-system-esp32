#include <Arduino.h>
#include "display.h"
#include "servo.h"
#include "relay.h"
#include "audio.h"
#include "physical_buttons.h"
#include "ir_control.h"
#include "system.h"
#include "menu.h"
#include "servo.h"

void setup() {    
    Serial.begin(115200);
    Serial.println("Setup started");
    systemSetup();
    relayPowerSetup();
    irSetup();
    physicalButtonsSetup();   
    displaySetup();
    i2sOutSetup();
    Serial.println("Setup done");
}

void loop() {
    //decodeNewRemote(); //uncomment this for decoding a new remote
    irStateLoop(currentIrState);
    servoLoop();
    menuStateLoop();
    systemStateLoop(currentSystemState);
}