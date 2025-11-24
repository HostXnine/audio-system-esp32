#include "display.h"
#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h> //core libary which is needed by the display specific library
#include "debug.h"

constexpr uint8_t SCREEN_ADDRESS = 0x3C;
constexpr uint16_t SCREEN_WIDTH = 128;
constexpr uint16_t SCREEN_HEIGHT = 64;
constexpr int16_t OLED_RESET = -1;

Adafruit_SH1106G Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void displaySetup() {
    debugln("Oled Setup");
    Display.begin(SCREEN_ADDRESS, true);
    Display.setContrast (0);
    Display.setTextColor(SH110X_WHITE);
}