#include "menu.h"
#include "display.h"
#include "servo.h"
#include "audio.h"
#include "system.h"
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include <BluetoothA2DPSink.h>
#include <Arduino.h>

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
    Serial.print("Menu: ");
    Serial.println(text);
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
    Serial.print("Menu: ");
    Serial.print(text1);
    Serial.print("; ");
    Serial.println(text2);
}

void displayClear() {
    Serial.println(" cleardisplay()");
    Display.clearDisplay();
    Serial.println(" display()");
    Display.display();
}

void menuState(MenuState currentMenuState) {
    switch (currentMenuState) {
        case MenuState::TV:
            Serial.println("menuState=TV");
            menuOneLine("TV",30, 12, 5);
            break;
        case MenuState::BLUETOOTH_CONNECTING:
            Serial.println("menuState=BLUETOOTH_CONNECTING");
            menuTwoLines("Bluetooth", 0, 10, "Connecting", 0, 40, 2);
            break;
        case MenuState::BLUETOOTH_CONNECTED:
            Serial.println("menuState=BLUETOOTH_CONNECTED");
            menuTwoLines("Bluetooth", 0, 10, "Connected", 0, 40, 2);
            break;
        case MenuState::RADIO:
            Serial.println("menuState=RADIO");
            menuTwoLines("Radio", 0, 10, currentRadioStationName, 0, 40, 2);
            break;
        case MenuState::AUX:
            Serial.println("menuState=AUX");
            menuOneLine("AUX",30, 10, 5);
            break;
        case MenuState::VOLUME:
            Serial.println("menuState=VOLUME");
            menuTwoLines("Volume", 0, 10, "Pos", 0, 40, 2);
            break;
        case MenuState::OFF:
            Serial.println("menuState=OFF");
            displayClear();
            break;
        default:
            break;
    }
}

void changeMenuState(SystemState systemState) {
    switch (systemState) {
        case TV_STATE:
            menuState(MenuState::TV);
            break;
        case BLUETOOTH_STATE:
            menuState(MenuState::BLUETOOTH_CONNECTING);
            break;
        case RADIO_STATE:
            menuState(MenuState::RADIO);
            break;
        case AUX_STATE:
            menuState(MenuState::AUX);
            break;
        case OFF_STATE:
            menuState(MenuState::OFF);
            break;
        default:
            break;
    }
}

void menuStateLoop() {
    static SystemState newSystemState = STATE_COUNT;
    if (newSystemState != currentSystemState) {
        changeMenuState(currentSystemState);
        newSystemState = currentSystemState;
    }
    if (currentSystemState == BLUETOOTH_STATE) {
        static bool connected = false;
        if (!a2dp_sink.is_connected() && !connected){
            menuState(MenuState::BLUETOOTH_CONNECTING);
            connected = true;
        }
        if (a2dp_sink.is_connected() && (connected)) {
            menuState(MenuState::BLUETOOTH_CONNECTED);
            connected = false;
        }
    }
}