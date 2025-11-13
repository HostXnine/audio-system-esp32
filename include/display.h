#pragma once
#include <Arduino.h>

constexpr uint8_t SCREEN_ADDRESS = 0x3C;
constexpr uint16_t SCREEN_WIDTH = 128;
constexpr uint16_t SCREEN_HEIGHT = 64;
constexpr int16_t OLED_RESET = -1;

class MenuClass {
public:
    void mainMenuText(const char mainName[10], int16_t x, int16_t y, int8_t size);
    void bluetoothMenu(const char mainName[10], const char status[10]);
    void volumeMenu();
    void radioMenu();
};

void displaySetup();