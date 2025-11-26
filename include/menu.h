#pragma once
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

extern MenuState currentMenuState;

void menuState(MenuState currentMenuState);
void menuUpdateRadioName();
void menuBluetoothConnect();