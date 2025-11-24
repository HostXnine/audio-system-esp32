#include "menu.h"
#include "display.h"
#include "audio.h"
#include "system.h"
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include "debug.h"

enum class MenuState : uint8_t {
    TV,
    BLUETOOTH_CONNECTING,
    BLUETOOTH_CONNECTED,
    RADIO,
    AUX,
    VOLUME,
    OFF,
    COUNT
};

MenuState currentMenuState = MenuState::TV;

void menuOneLine(const char* text, int16_t x, int16_t y, int8_t size) {
    Display.clearDisplay();
    Display.setCursor(x, y);
    Display.setTextSize(size);
    Display.println(text);
    Display.display();
    debug("Menu: ");
    debugln(text);
}

void menuTwoLines(const char* text1, int16_t x1, int16_t y1,
const char* text2, int16_t x2, int16_t y2, int8_t size) {
    Display.clearDisplay();
    Display.setCursor(x1, y1);
    Display.setTextSize(2);
    Display.println(text1);
    Display.setCursor(x2, y2);
    Display.println(text2);
    Display.display();
    debug("Menu: ");
    debug(text1);
    debug("; ");
    debugln(text2);
}

void displayClear() {
    debugln(" cleardisplay()");
    Display.clearDisplay();
    debugln(" display()");
    Display.display();
}

void menuState(MenuState currentMenuState) {
    switch (currentMenuState) {
        case MenuState::TV:
            debugln("menuState=TV");
            menuOneLine("TV",30, 12, 5);
            break;
        case MenuState::BLUETOOTH_CONNECTING:
            debugln("menuState=BLUETOOTH_CONNECTING");
            menuTwoLines("Bluetooth", 0, 10, "Connecting", 0, 40, 2);
            break;
        case MenuState::BLUETOOTH_CONNECTED:
            debugln("menuState=BLUETOOTH_CONNECTED");
            menuTwoLines("Bluetooth", 0, 10, "Connected", 0, 40, 2);
            break;
        case MenuState::RADIO:
            debugln("menuState=RADIO");
            menuTwoLines("Radio", 0, 10, currentRadioStationName, 0, 40, 2);
            break;
        case MenuState::AUX:
            debugln("menuState=AUX");
            menuOneLine("AUX",30, 10, 5);
            break;
        case MenuState::VOLUME:
            debugln("menuState=VOLUME");
            menuTwoLines("Volume", 0, 10, "Pos", 0, 40, 2);
            break;
        case MenuState::OFF:
            debugln("menuState=OFF");
            displayClear();
            break;
        default:
            break;
    }
}

void changeMenuState(SystemState systemState) {
    switch (systemState) {
        case SystemState::TV_STATE:
            menuState(MenuState::TV);
            break;
        case SystemState::BLUETOOTH_STATE:
            menuState(MenuState::BLUETOOTH_CONNECTING);
            break;
        case SystemState::RADIO_STATE:
            menuState(MenuState::RADIO);
            break;
        case SystemState::AUX_STATE:
            menuState(MenuState::AUX);
            break;
        case SystemState::OFF_STATE:
            menuState(MenuState::OFF);
            break;
        default:
            break;
    }
}

void menuUpdateRadioName() {
    if (currentSystemState == SystemState::RADIO_STATE) {
        menuState(MenuState::RADIO);
    }
}

void menuBluetoothConnect() {
    static bool connected = false;

    if (currentSystemState == SystemState::BLUETOOTH_STATE) {
        if (!a2dpIsConnected() && !connected){
            menuState(MenuState::BLUETOOTH_CONNECTING);
            connected = true;
        }
        if (a2dpIsConnected() && (connected)) {
            menuState(MenuState::BLUETOOTH_CONNECTED);
            connected = false;
        }
    }
}

void menuStateLoop() {
    static SystemState newSystemState = SystemState::STATE_COUNT;
    
    if (newSystemState != currentSystemState) {
        changeMenuState(currentSystemState);
        newSystemState = currentSystemState;
    }
    menuBluetoothConnect();
}

