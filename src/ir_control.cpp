#include "ir_control.h"
#include "buttons.h"
#include "stepper.h"
#define DECODE_NEC
//#define DECODE_DENON //includes sharp. Has to be called before IRremote.hpp. Comment this when adding a new remote.
#include <IRremote.hpp>
#include "debug.h"

constexpr uint8_t IR_RECEIVE_PIN = 19;

unsigned long debounceDelayIr = 500;
static bool initIrDebounce = false;

IrState currentIrState = IrState::IR_RECEIVE_STATE;

void irSetup() {
    debugln("IR setup");
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
    static unsigned long lastDebounceTime = 0;
    if (!initIrDebounce) {
        initIrDebounce = true;
        lastDebounceTime = millis();
        debugln(" Debounce start ");
    }
    if (((millis() - lastDebounceTime) > (debounceDelay)) && (initIrDebounce)) {
        initIrDebounce = false;
        debugln("Debounce ends");
        debounceDelay = 500;
        changeIrState(); //changes state!!!
    }
}

void changeIrState() {
    switch(currentIrState) {
        case IrState::IR_RECEIVE_STATE: {
            bool valid = validButton(currentButton);
            if (valid) {
                debugln("changeIrState = IR_DEBOUNCE_STATE");
                currentIrState = IrState::IR_DEBOUNCE_STATE;
            } else { 
                debugln("changeIrState = IR_RESET_STATE");
                currentIrState = IrState::IR_RESET_STATE;
            }
            break;
            }
        case IrState::IR_DEBOUNCE_STATE:
            if (!initIrDebounce) {
                debugln("changeIrState = IR_RECEIVE_STATE");
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
            //debugln("IR_RECEIVE_STATE");
            if (IrReceiver.decode()) {
                currentButton = (static_cast<Buttons>(IrReceiver.decodedIRData.command));
                if (currentButton == Buttons::VOLUME_UP_BUTTON) {
                    changeStepperState(StepperState::STEPPER_UP);
                    currentButton = Buttons::NONE_BUTTON;
                    IrReceiver.resume();
                } else if (currentButton == Buttons::VOLUME_DOWN_BUTTON) {
                    changeStepperState(StepperState::STEPPER_DOWN);
                    currentButton = Buttons::NONE_BUTTON;
                    IrReceiver.resume();
                } else {
                    buttonInput(currentButton);
                }
            }
            break;
        case IrState::IR_DEBOUNCE_STATE:
            //debugln("IR_DEBOUNCE_STATE");
            debounceIr(debounceDelayIr); //self sets to IR_RESET_STATE
            break;
        case IrState::IR_RESET_STATE:
            //debugln("IR_RESET_STATE");
            IrReceiver.resume();
            changeIrState(); //self sets to IR_RECEIVE_STATE
            break;
  }
}


