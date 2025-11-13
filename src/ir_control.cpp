#include "ir_control.h"
#include <Arduino.h>
#define DECODE_NEC
//#define DECODE_DENON //includes sharp. Has to be called before IRremote.hpp. Comment this when adding a new remote.
#include <IRremote.hpp>

enum Buttons {
    /* Buttons for Sharp and NEC
    Buttons for Sharp remote it uses DECODE_DENON:
    NET 149      |   ON/OFF 233
        1 254 | 2 253 |   3 252
        4 251 | 5 250 |   6 249
        7 248 | 8 247 |   9 246
    <--> 216 | 0 254 |->[] 236
    <> 55 | v/oo 231 | i+ 228 | FAV 86
    vol+ 235  | 3D 76  | P^ 238
    vol- 234  | EPG 92 | Pv 237 
    mute 232 | MODE/AV 7 | ECO 200 | END 10
            ^ 168
        < 40 |OK 173| > 39
            v 223
    MENU 57            backArrow 27              
        red 183 |    green 182 |   yellow 181 |    blue 180
    teletext 203 | subtitles 96 |   ATV/DTV 95 |    RADIO 91
        SOURCE 255 |      REC 250 | REC STOP 249 | USB REC 194
            << 252 |       [] 253 |     >/II 254 |      >> 251 

    Buttons on VCR remote. I uses DECODE_NEC
    Operate 20
    1  5 | 2  6 | 3  7
    4 12 | 5 13 | 6 14
    7 15 | 8 28 | 9 29
    v 25 | 0  4 | ^ 24
    << 2 | >  8 | >> 3   | [] inactive
    ||11 | [] 1 | ||> 16 | O  inactive
    When pressed [] and O simultanious it is 9
    */
    //General (NEC)
    MENU_BUTTON = 5,
    POWER_BUTTON = 20,
    VOLUME_UP_BUTTON = 24,
    VOLUME_DOWN_BUTTON = 25,
    DEBUG_BUTTON = 500,
    //Player control (NEC)
    NEXT_BUTTON = 3,
    PREVIOUS_BUTTON = 2,
    STOP_BUTTON = 1,
    PAUSE_BUTTON = 11,
    PLAY_BUTTON = 8,
    NONE_BUTTON = 0,
    /*  //General (keyboard)
    MENU = 53, // 5
    POWER = 48, // 0
    VOLUME_UP = 43, // -
    VOLUME_DOWN = 45, // +
    DEBUG = 42, // *
    //Player control (keyboard)
    NEXT = 54, // 6
    PREVIOUS = 52, // 4
    STOP = 49, // 1
    PAUSE = 51, // 3
    PLAY = 36 // home */
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
            //changeSystemState();
            break;
        case POWER_BUTTON:
            Serial.println("button = POWER_BUTTON");
            changeIrState();
            //changeOnOffSystemState();
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

