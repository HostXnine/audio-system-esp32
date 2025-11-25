#include "system.h"
#include "servo.h"
#include "audio.h"
#include "relay.h"
#include "menu.h"
#include "debug.h"

esp_reset_reason_t resetReason = esp_reset_reason();

RTC_NOINIT_ATTR SystemState currentSystemState = SystemState::TV_STATE;

void detachAll() {
    debugln(" Detaching...");
    servoDetach();
    detachAudio();
}

void restartSystem() {
    relayRestart();
    detachAll();
    debugln(" Restarting now...");
    esp_restart();
}

void systemStateSetup(SystemState state) { //has to run only once
    switch (state) {
        case SystemState::TV_STATE:
            debugln("TV_STATE SETUP");
            menuState(MenuState::TV);
            i2sInSetup();
            i2sOutSetup();
            break;
        case SystemState::BLUETOOTH_STATE:
            debugln("BLUETOOTH_STATE SETUP");
            menuState(MenuState::BLUETOOTH_CONNECTING);
            a2dpStart();
            i2sOutSetup();
            break;
        case SystemState::RADIO_STATE:
            debugln("RADIO_STATE SETUP");
            menuState(MenuState::RADIO);
            playerBegin();
            i2sOutSetup();
            break;
        case SystemState::AUX_STATE:
            debugln("AUX_STATE SETUP");
            menuState(MenuState::AUX);
            relayAuxState();
            break;
        case SystemState::OFF_STATE:
            debugln("OFF_STATE SETUP");
            menuState(MenuState::OFF);
            detachAll();
            relayOffState();
            debugln("OFF  ");        
            break;
        default:
            break;
    }
}

void systemStateLoop(SystemState state) {
    switch (state) {
        case SystemState::TV_STATE:
            //debugln("TV_STATE LOOP");
            copierInOutCopy();
            break;
        case SystemState::BLUETOOTH_STATE:
            menuBluetoothConnect();
        case SystemState::RADIO_STATE:
            //debugln("RADIO_STATE LOOP");
            playerCopy();
            break;
        default:
            break;
    }
}

void setSystemState (SystemState newState) {
    if (newState == SystemState::OFF_STATE) {
        debugln("setSystemState == OFF_STATE");
        currentSystemState = SystemState::OFF_STATE;
        systemStateSetup(currentSystemState);
        return;
    }  
    if (newState != currentSystemState) {
        debugln("setSystemState has changed");
        currentSystemState = newState;
        restartSystem();
    } else {
        debugln("setSystemState runs systemStateSetup");
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
        debugln("changeSystemState != AUX_STATE");
        setSystemState(nextSystemState(currentSystemState, 1));
    } else {
        debugln("changeSystemState == AUX_STATE");
        setSystemState(nextSystemState(currentSystemState, 2));
    }
}

void changeOnOffSystemState() {
    if (currentSystemState != SystemState::OFF_STATE){
        debugln("changeOnOffSystemState() != OFF_STATE");
        setSystemState(SystemState::OFF_STATE);
    } else {
        debugln("changeOnOffSystemState() == OFF_STATE");
        setSystemState(static_cast<SystemState>(0)); //sets to first element in enum
    }
}

void systemSetup() { 
    if (resetReason != ESP_RST_SW) {
        debugln("RTC variabke setup");
        setSystemState(static_cast<SystemState>(0)); //sets to first element in enum
        debugln("systemSetup() setSystemState(0)");
    } else {
        setSystemState(currentSystemState);
        debugln("systemSetup() setSystemState(currentSystemState)");
    }
}