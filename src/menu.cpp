#include "menu.h"
#include "display.h"
#include "audio.h"
#include "system.h"
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include "debug.h"

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
    Display.setTextSize(size);
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
    Display.clearDisplay();
    Display.display();
}

static const char* radioStatusTitle(RadioStatus status) {
    switch (status) {
        case RadioStatus::CONNECTING_WIFI: return "WiFi...";
        case RadioStatus::NO_WIFI:         return "No WiFi";
        case RadioStatus::CONNECTING:      return "Tuning...";
        case RadioStatus::STATION_FAILED:  return "No signal";
        default:                           return "Radio";
    }
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
            menuTwoLines(radioStatusTitle(radioGetStatus()), 0, 10, currentRadioStationName, 0, 40, 2);
            break;
        case MenuState::AUX:
            debugln("menuState=AUX");
            menuOneLine("AUX",20, 12, 5);
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

void menuUpdateRadioName() {
    if (currentSystemState == SystemState::RADIO_STATE) {
        menuState(MenuState::RADIO);
    }
}

void menuBluetoothConnect() {
    static bool connected = true;
    if (!a2dpIsConnected() && !connected){
        menuState(MenuState::BLUETOOTH_CONNECTING);
        connected = true;
    }
    if (a2dpIsConnected() && (connected)) {
        menuState(MenuState::BLUETOOTH_CONNECTED);
        connected = false;
    }
}

// Called from loop() in RADIO_STATE: redraws the display when the radio task's status changes.
void menuRadioStatus() {
    static RadioStatus shown = RadioStatus::IDLE;
    RadioStatus now = radioGetStatus();
    if (now != shown) {
        shown = now;
        menuState(MenuState::RADIO);
    }
}
