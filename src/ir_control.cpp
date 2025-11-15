#include "ir_control.h"
#include "buttons.h"
#include <Arduino.h>
#define DECODE_NEC
//#define DECODE_DENON //includes sharp. Has to be called before IRremote.hpp. Comment this when adding a new remote.
#include <IRremote.hpp>

constexpr uint8_t IR_RECEIVE_PIN = 19;

static unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 500;
static bool initIrDebounce = false;

IrState currentIrState = IrState::IR_RECEIVE_STATE;

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
        debounceDelay = 500;
        changeIrState(); //changes state!!!
    }
}

void changeIrState() {
    switch(currentIrState) {
        case IrState::IR_RECEIVE_STATE: {
            bool valid = validButton(currentButton);
            if (valid) {
                Serial.println("changeIrState = IR_DEBOUNCE_STATE");
                currentIrState = IrState::IR_DEBOUNCE_STATE;
            } else { 
                Serial.println("changeIrState = IR_RESET_STATE");
                currentIrState = IrState::IR_RESET_STATE;
            }
            break;
            }
        case IrState::IR_DEBOUNCE_STATE:
            if (!initIrDebounce) {
                Serial.println("changeIrState = IR_RECEIVE_STATE");
                currentIrState = IrState::IR_RESET_STATE;
            }
            break;
        case IrState::IR_RESET_STATE:
            currentButton = Buttons::NONE_BUTTON;
            currentIrState = IrState::IR_RECEIVE_STATE;
            break;
    }
}

void irStateLoop(IrState state) {
    switch (state) {
        case IrState::IR_RECEIVE_STATE:
            //Serial.println("IR_RECEIVE_STATE");
            if (IrReceiver.decode()) {
                currentButton = (static_cast<Buttons>(IrReceiver.decodedIRData.command));
                buttonInput(currentButton);
            }
            break;
        case IrState::IR_DEBOUNCE_STATE:
            //Serial.println("IR_DEBOUNCE_STATE");
            debounceIr(debounceDelay); //self sets to IR_RESET_STATE
            break;
        case IrState::IR_RESET_STATE:
            //Serial.println("IR_RESET_STATE");
            IrReceiver.resume();
            changeIrState(); //self sets to IR_RECEIVE_STATE
            break;
  }
}


