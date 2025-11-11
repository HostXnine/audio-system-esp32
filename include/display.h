#pragma once

constexpr int SCREEN_ADDRESS = 0x3C;
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;

class Adafruit_SH1106G;
extern Adafruit_SH1106G display;

class MenuClass;
extern MenuClass Menus;

void oledSetup();