#include "ir_control.h"
#include "system.h"
#include <Arduino.h>
#define DECODE_NEC
//#define DECODE_DENON //includes sharp. Has to be called before IRremote.hpp. Comment this when adding a new remote.
#include <IRremote.hpp>

enum Buttons : uint8_t {
    MENU_BUTTON = 5,
    POWER_BUTTON = 20,
    VOLUME_UP_BUTTON = 24,
    VOLUME_DOWN_BUTTON = 25,
    DEBUG_BUTTON = 240,
    NEXT_BUTTON = 3,
    PREVIOUS_BUTTON = 2,
    STOP_BUTTON = 1,
    PAUSE_BUTTON = 11,
    PLAY_BUTTON = 8,
    NONE_BUTTON = 0,
};

constexpr uint8_t IR_RECEIVE_PIN = 19;

static unsigned long lastDebounceTime = 0;
static unsigned long debounceDelay;
static bool initIrDebounce = false;
Buttons currentButton = NONE_BUTTON;

IrState currentIrState = IR_RECEIVE_STATE;

static void changeIrState();
static void buttonInput(Buttons &button);

void irSetup() {
    Serial.println("IR setup");
    IrReceiver.begin(IR_RECEIVE_PIN);
}

void decodeNewRemote() { //only used when decoding a new remote
    if (IrReceiver.decode()) {
        Serial.print("IrReceiver.decodedIRData.command=");
        Serial.print(IrReceiver.decodedIRData.command);
        Serial.print(" Protocol=");
        Serial.print(IrReceiver.decodedIRData.protocol);
        Serial.print(" ProtocolName=");
        Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));
        IrReceiver.resume();
    }
}

void debounceIr(unsigned long debounceDelay = 500) {
    if (!initIrDebounce) {
        initIrDebounce = true;
        lastDebounceTime = millis();
        Serial.println(" Debounce start ");
    }
    if (((millis() - lastDebounceTime) > (debounceDelay)) && (initIrDebounce)) {
        initIrDebounce = false;
        Serial.println("Debounce ends");
        changeIrState(); //changes state!!!
    }
}

bool validButton(Buttons button) {
    switch(button) {
        case MENU_BUTTON:
            Serial.println("validButton(MENU_BUTTON)");
            return true;
            break;
        case POWER_BUTTON:
            Serial.println("validButton(POWER_BUTTON)");
            return true;
            break;
        default:
            Serial.println("validButton(default)");
            return false;
        break;
    }
}

void changeIrState() {
    switch(currentIrState) {
        case IR_RECEIVE_STATE: {
        bool valid = validButton(currentButton);
        if (valid) {
            Serial.println("changeIrState = IR_DEBOUNCE_STATE");
            currentIrState = IR_DEBOUNCE_STATE;
        } else { 
            Serial.println("changeIrState = IR_RESET_STATE");
            currentIrState = IR_RESET_STATE;
        }
        break;
        }
        case IR_DEBOUNCE_STATE:
        if (!initIrDebounce) {
            Serial.println("changeIrState = IR_RECEIVE_STATE");
            currentIrState = IR_RESET_STATE;
        }
        break;
        case IR_RESET_STATE:
        currentButton = NONE_BUTTON;
        currentIrState = IR_RECEIVE_STATE;
        break;
    }
}

void irStateLoop(IrState state) {
    switch (state) {
        case IR_RECEIVE_STATE:
            Serial.println("IR_RECEIVE_STATE");
            if (IrReceiver.decode()) {
                currentButton = (static_cast<Buttons>(IrReceiver.decodedIRData.command));
                buttonInput(currentButton);
            }
            break;
        case IR_DEBOUNCE_STATE:
            Serial.println("IR_DEBOUNCE_STATE");
            debounceIr(debounceDelay); //self sets to IR_RESET_STATE
            break;
        case IR_RESET_STATE:
            Serial.println("IR_RESET_STATE");
            IrReceiver.resume();
            changeIrState(); //self sets to IR_RECEIVE_STATE
            break;
  }
}

void buttonInput(Buttons &button) {
    switch(button) { //button press should be run only once!
        case MENU_BUTTON:
            Serial.println("button = MENUT_BUTTON");
            debounceDelay = 1000; //if don't set this then the default value is 500 ms
            changeIrState();
            changeSystemState();
            break;
        case POWER_BUTTON:
            Serial.println("button = POWER_BUTTON");
            changeIrState();
            changeOnOffSystemState();
            break;
        default:
            Serial.println("button = non-valid button");
            changeIrState();
            break;
    }
    Serial.println("button = NONE_BUTTON");
    button = NONE_BUTTON;
}

/*void playerControl() {
  int8_t sizeOfUrls = (sizeof(urls) / sizeof(urls[0]));

  switch(irReceivedData) {
    case NEXT:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.next();
    } 
    else if (player.isActive()) {
      player.next();
      currentStation = ((currentStation + 1) % sizeOfUrls);
      Menus.radioMenu();
    } 
    break;
    case PREVIOUS:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.previous();
    } 
    else if (player.isActive()) {
      player.previous();
      currentStation = ((currentStation - 1 + sizeOfUrls) % sizeOfUrls);
      Menus.radioMenu();
    }
    break;
    case PLAY:
    if (a2dp_sink.is_connected()) {
      if (!paused) {
        a2dp_sink.pause();
        paused = true;
      }
      if (paused) {
        a2dp_sink.play();
        paused = false;
      }
    } 
    break;
  }
}
*/

