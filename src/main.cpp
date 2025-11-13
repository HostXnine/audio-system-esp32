#include <Arduino.h>
#include "display.h"
#include "servo.h"
#include "relay.h"
#include "audio.h"
#include "physical_buttons.h"
#include "ir_control.h"

/*To do list:
- add url radio connection timeout
- fix broken functionality when bluetooth is selected
- fix spageti code in tasks and menuControl
- optimize deoubce for remote control
- maybe split the remote and remote debounce functionality since for some buttons it has to be set at a different timeout
*/

// Tasks and menu control global variables
esp_reset_reason_t resetReason = esp_reset_reason();
uint32_t myLastTime; //for implementing delays. For some reason it has to be a global variable otherwise it doesn't work.


enum SystemState {
    TV_STATE,
    BLUETOOTH_STATE,
    RADIO_STATE,
    AUX_STATE,
    OFF_STATE, //keep it always before the last item
    STATE_COUNT // keep it always last
};

RTC_NOINIT_ATTR enum SystemState currentSystemState = TV_STATE;

void detachAll() {
    Serial.println(" Writing servo position to RTC... ");
    postitionRtc = position;
    Serial.println(" Detaching...");
    servoDetach();
    Serial.println(" cleardisplay()");
    //display.clearDisplay();
    Serial.println(" display()");
    //display.display();
    detachAudio();
}

void restartSystem() {
    Serial.println(" digitalWrite ON_OFF_PIN");
    if (digitalRead(ON_OFF_5V_PIN) == LOW) {
        digitalWrite(ON_OFF_5V_PIN, HIGH);
    }
    if (digitalRead(ON_OFF_AC_PIN) == LOW) {
        digitalWrite(ON_OFF_AC_PIN, HIGH);
    }
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
            if (digitalRead(AUX_LEFT_PIN) == LOW || digitalRead(AUX_RIGHT_PIN) == LOW) {
            digitalWrite(AUX_LEFT_PIN, HIGH);
            digitalWrite(AUX_RIGHT_PIN, HIGH);
            }
            break;
        case OFF_STATE:
            Serial.println("OFF_STATE SETUP");
            detachAll();
            digitalWrite(AUX_LEFT_PIN, LOW);
            digitalWrite(AUX_RIGHT_PIN, LOW);
            digitalWrite(ON_OFF_5V_PIN, LOW);
            digitalWrite(ON_OFF_AC_PIN, LOW);
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
        setSystemState(static_cast<SystemState>(STATE_COUNT % STATE_COUNT)); //with other words set to 0
    }
}


void setup() {    
    //Debugging
    Serial.begin(115200);
    Serial.println("Setup started");
    
    //System state setup  
    if (resetReason != ESP_RST_SW) {
        Serial.println("RTC variabke setup");
        setSystemState(static_cast<SystemState>(STATE_COUNT % STATE_COUNT)); //with other words set to 0
        Serial.println("setup() setSystemState(0)");
    } else {
        setSystemState(currentSystemState);
        Serial.println("setup() setSystemState(currentSystemState)");
    }
    
    servoSetup();
    relayPowerSetup();
    relayAuxSetup();
    irSetup();
    physicalButtonsSetup();   
    displaySetup();
    i2sOutSetup();
    i2sInSetup();

    Serial.println("Setup done");
}

void loop() {
    //decodeNewRemote(); //uncomment this for decoding a new remote
    irStateLoop(currentIrState);
    systemStateLoop(currentSystemState);
}