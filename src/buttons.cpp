#include "buttons.h"
#include "ir_control.h"
#include "system.h"
#include "audio.h"
#include "menu.h"

Buttons currentButton = Buttons::NONE_BUTTON;

bool validButton(Buttons button) {
    switch(button) {
        case Buttons::MENU_BUTTON:
        case Buttons::POWER_BUTTON:
        case Buttons::VOLUME_UP_BUTTON:
        case Buttons::VOLUME_DOWN_BUTTON:
        case Buttons::DEBUG_BUTTON:
        case Buttons::NEXT_BUTTON:
        case Buttons::PREVIOUS_BUTTON:
        case Buttons::STOP_BUTTON:
        case Buttons::PAUSE_BUTTON:
        case Buttons::PLAY_BUTTON:
            Serial.println("validButton(true)");
            return true;
            break;
        default:
            Serial.println("validButton(default)");
            return false;
            break;
    }
}

void buttonInput(Buttons &button) {
    switch(button) { //button press should be run only once!
        case Buttons::MENU_BUTTON:
            Serial.println("button = MENUT_BUTTON");
            //debounceDelay = 1000; //if don't set this then the default value is 500 ms
            changeIrState();
            changeSystemState();
            break;
        case Buttons::POWER_BUTTON:
            Serial.println("button = POWER_BUTTON");
            changeIrState();
            changeOnOffSystemState();
            break;
        case Buttons::NEXT_BUTTON:
            audioNext();
            menuUpdateRadioName();
            changeIrState();
            break;
        case Buttons::PREVIOUS_BUTTON:
            audioPrevious();
            menuUpdateRadioName();
            changeIrState();
            break;
        case Buttons::PLAY_BUTTON:
            audioPlayPause();
            changeIrState();
            break;
        default:
            Serial.println("button = non-valid button");
            changeIrState();
            break;
    }
    Serial.println("button = NONE_BUTTON");
    button = Buttons::NONE_BUTTON;
}