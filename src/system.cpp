#include "system.h"
#include "servo.h"
#include "audio.h"
#include "relay.h"
#include "menu.h"

esp_reset_reason_t resetReason = esp_reset_reason();

RTC_NOINIT_ATTR SystemState currentSystemState = SystemState::TV_STATE;

void detachAll() {
    Serial.println(" Writing servo position to RTC... ");
    postitionRtc = position;
    Serial.println(" Detaching...");
    servoDetach();
    detachAudio();
}

void restartSystem() {
    relayRestart();
    detachAll();
    Serial.println(" Restarting now...");
    esp_restart();
}

void systemStateSetup(SystemState state) { //has to run only once
    switch (state) {
        case SystemState::TV_STATE:
            Serial.println("TV_STATE SETUP");
            i2sInSetup();
            break;
        case SystemState::BLUETOOTH_STATE:
            Serial.println("BLUETOOTH_STATE SETUP");
            a2dpStart();
            break;
        case SystemState::RADIO_STATE:
            Serial.println("RADIO_STATE SETUP");
            playerBegin();
            break;
        case SystemState::AUX_STATE:
            Serial.println("AUX_STATE SETUP");
            relayAuxState();
            break;
        case SystemState::OFF_STATE:
            Serial.println("OFF_STATE SETUP");
            detachAll();
            relayOffState();
            Serial.println("OFF  ");        
            break;
        default:
            break;
    }
}

void systemStateLoop(SystemState state) {
    switch (state) {
        case SystemState::TV_STATE:
            //Serial.println("TV_STATE LOOP");
            copierInOutCopy();
            break;
        case SystemState::RADIO_STATE:
            //Serial.println("RADIO_STATE LOOP");
            playerCopy();
            break;
        default:
            break;
    }
}

void setSystemState (SystemState newState) {
    if (newState == SystemState::OFF_STATE) {
        Serial.println("setSystemState == OFF_STATE");
        currentSystemState = SystemState::OFF_STATE;
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

inline SystemState nextSystemState(SystemState state, uint8_t step) {
    return static_cast<SystemState>(
        (static_cast<uint8_t>(state) + step) % static_cast<uint8_t>(SystemState::STATE_COUNT)
    );
}

void changeSystemState() { //rotates through system states
    if (currentSystemState != SystemState::AUX_STATE) {
        Serial.println("changeSystemState != AUX_STATE");
        setSystemState(nextSystemState(currentSystemState, 1));
    } else {
        Serial.println("changeSystemState == AUX_STATE");
        setSystemState(nextSystemState(currentSystemState, 2));
    }
}

void changeOnOffSystemState() {
    if (currentSystemState != SystemState::OFF_STATE){
        Serial.println("changeOnOffSystemState() != OFF_STATE");
        setSystemState(SystemState::OFF_STATE);
    } else {
        Serial.println("changeOnOffSystemState() == OFF_STATE");
        setSystemState(static_cast<SystemState>(0)); //sets to first element in enum
    }
}

void systemSetup() { 
    if (resetReason != ESP_RST_SW) {
        Serial.println("RTC variabke setup");
        setSystemState(static_cast<SystemState>(0)); //sets to first element in enum
        Serial.println("systemSetup() setSystemState(0)");
    } else {
        setSystemState(currentSystemState);
        Serial.println("systemSetup() setSystemState(currentSystemState)");
    }
}