#include "system.h"
#include "servo.h"
#include "audio.h"
#include "relay.h"
#include "menu.h"

esp_reset_reason_t resetReason = esp_reset_reason();
uint32_t myLastTime; //for implementing delays. For some reason it has to be a global variable otherwise it doesn't work.

RTC_NOINIT_ATTR SystemState currentSystemState = TV_STATE;

void detachAll() {
    Serial.println(" Writing servo position to RTC... ");
    postitionRtc = position;
    Serial.println(" Detaching...");
    servoDetach();
    menuState(MenuState::OFF);
    detachAudio();
}

void restartSystem() {
    relayPowerRestart();
    detachAll();
    Serial.println(" Restarting now...");
    esp_restart();
}

void systemStateSetup(SystemState state) { //has to run only once
    switch (state) {
        case TV_STATE:
            Serial.println("TV_STATE SETUP");
            break;
        case BLUETOOTH_STATE:
            Serial.println("BLUETOOTH_STATE SETUP");
            a2dpSinkStart();
            break;
        case RADIO_STATE:
            Serial.println("RADIO_STATE SETUP");
            playerBegin();
            break;
        case AUX_STATE:
            Serial.println("AUX_STATE SETUP");
            relayAuxState();
            break;
        case OFF_STATE:
            Serial.println("OFF_STATE SETUP");
            detachAll();
            relayOffState();
            Serial.println("OFF  ");        
        break;
    }
}

void systemStateLoop(SystemState state) {
    switch (state) {
        case TV_STATE:
            Serial.println("TV_STATE LOOP");
            copierInOutCopy();
            break;
        case RADIO_STATE:
            Serial.println("RADIO_STATE LOOP");
            playerCopy();
            break;
        default:
            break;
    }
}

void setSystemState (SystemState newState) {
    if (newState == OFF_STATE) {
        Serial.println("setSystemState == OFF_STATE");
        currentSystemState = OFF_STATE;
        systemStateSetup(currentSystemState);
        return;
    }  
    if (newState != currentSystemState) {
        Serial.println("setSystemState has changed");
        currentSystemState = newState;
        restartSystem();
    } else {
        Serial.println("setSystemState runs systemStateSetup");
        systemStateSetup(currentSystemState);
    }
}

void changeSystemState() { //rotates through system states
    if (currentSystemState != AUX_STATE) {
        Serial.println("changeSystemState != AUX_STATE");
        setSystemState(static_cast<SystemState>((currentSystemState + 1) % STATE_COUNT));
    } else {
        Serial.println("changeSystemState == AUX_STATE");
        setSystemState(static_cast<SystemState>((currentSystemState + 2) % STATE_COUNT));
    }
}

void changeOnOffSystemState() {
    if (currentSystemState != OFF_STATE){
        Serial.println("changeOnOffSystemState() != OFF_STATE");
        setSystemState(OFF_STATE);
    } else {
        Serial.println("changeOnOffSystemState() == OFF_STATE");
        setSystemState(TV_STATE); //with other words set to 0
    }
}

void systemSetup() { 
    if (resetReason != ESP_RST_SW) {
        Serial.println("RTC variabke setup");
        setSystemState(TV_STATE); //with other words set to 0
        Serial.println("systemSetup() setSystemState(0)");
    } else {
        setSystemState(currentSystemState);
        Serial.println("systemSetup() setSystemState(currentSystemState)");
    }
}