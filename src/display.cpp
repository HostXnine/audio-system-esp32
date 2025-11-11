#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include "display.h"

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void oledSetup() {
  Serial.println("Oled Setup");
  display.begin(SCREEN_ADDRESS, true);
  display.setContrast (0);
  display.setTextColor(SH110X_WHITE);
}

class MenuClass {
  public:
  void mainMenuText(const char mainName[10], int16_t x, int16_t y, int8_t size) {
    display.clearDisplay();
    display.setCursor(x, y);
    display.setTextSize(size);
    display.println(mainName);
    display.display(); 
    Serial.println(mainName);
  }
  void bluetoothMenu(const char mainName[10], const char status[10]) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.setTextSize(2);
    display.println(mainName);
    display.setCursor(0, 40);
    display.println(status);
    display.display(); 
    Serial.println(mainName);
  }
  void volumeMenu() {
    display.clearDisplay();
    display.setCursor(12, 20);
    display.setTextSize(3);
    //display.println(position);
    display.display();
    //Serial.println(position);
  }
  void radioMenu() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println("Radio");
    display.setCursor(0, 30);
    //display.println(radioStationNames[currentStation]);
    display.display();
    Serial.println("Radio");
    //Serial.println(radioStationNames[currentStation]);
  }
};