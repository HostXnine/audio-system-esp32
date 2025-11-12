#include "display.h"
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h> //core libary which is needed by the display specific library

static Adafruit_SH1106G Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MenuClass Menus;

void oledSetup() {
  Serial.println("Oled Setup");
  Display.begin(SCREEN_ADDRESS, true);
  Display.setContrast (0);
  Display.setTextColor(SH110X_WHITE);
}

void MenuClass::mainMenuText(const char mainName[10], int16_t x, int16_t y, int8_t size) {
    Display.clearDisplay();
    Display.setCursor(x, y);
    Display.setTextSize(size);
    Display.println(mainName);
    Display.display(); 
    Serial.println(mainName);
  }
void MenuClass::bluetoothMenu(const char mainName[10], const char status[10]) {
    Display.clearDisplay();
    Display.setCursor(0, 10);
    Display.setTextSize(2);
    Display.println(mainName);
    Display.setCursor(0, 40);
    Display.println(status);
    Display.display(); 
    Serial.println(mainName);
  }
void MenuClass::volumeMenu() {
    Display.clearDisplay();
    Display.setCursor(12, 20);
    Display.setTextSize(3);
    //display.println(position);
    Display.display();
    //Serial.println(position);
  }
void MenuClass::radioMenu() {
    Display.clearDisplay();
    Display.setCursor(0, 0);
    Display.setTextSize(2);
    Display.println("Radio");
    Display.setCursor(0, 30);
    //display.println(radioStationNames[currentStation]);
    Display.display();
    Serial.println("Radio");
    //Serial.println(radioStationNames[currentStation]);
  }