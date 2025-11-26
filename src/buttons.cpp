#include "buttons.h"
#include "ir_control.h"
#include "system.h"
#include "audio.h"
#include "menu.h"
#include "servo.h"
#include "debug.h"

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
            debugln("validButton(true)");
            return true;
            break;
        default:
            debugln("validButton(default)");
            return false;
            break;
    }
}

void buttonInput(Buttons &button) {
    switch(button) { //button press should be run only once!
        case Buttons::MENU_BUTTON:
            debugln("button = MENUT_BUTTON");
            //debounceDelay = 1000; //if don't set this then the default value is 500 ms
            changeIrState();
            changeSystemState();
            break;
        case Buttons::POWER_BUTTON:
            debugln("button = POWER_BUTTON");
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
        case Buttons::VOLUME_UP_BUTTON:
            debounceDelay = 0;
            changeServoState(ServoState::SERVO_UP);
            changeIrState();
            break;
        case Buttons::VOLUME_DOWN_BUTTON:
            debounceDelay = 0;
            changeServoState(ServoState::SERVO_DOWN);
            changeIrState();
            break;
        default:
            debugln("button = non-valid button");
            changeIrState();
            break;
    }
    debugln("button = NONE_BUTTON");
    button = Buttons::NONE_BUTTON;
}