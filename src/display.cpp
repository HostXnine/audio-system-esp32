#include "display.h"
#include "servo.h"
#include "audio.h"
#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h> //core libary which is needed by the display specific library

constexpr uint8_t SCREEN_ADDRESS = 0x3C;
constexpr uint16_t SCREEN_WIDTH = 128;
constexpr uint16_t SCREEN_HEIGHT = 64;
constexpr int16_t OLED_RESET = -1;

static Adafruit_SH1106G Display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

class MenuClass {
public:
    void mainMenuText(const char mainName[10], int16_t x, int16_t y, int8_t size);
    void bluetoothMenu(const char mainName[10], const char status[10]);
    void volumeMenu();
    void radioMenu();
};
static MenuClass Menus;

void displaySetup() {
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
    Display.println(position);
    Display.display();
    Serial.println(position);
}
void MenuClass::radioMenu() {
    Display.clearDisplay();
    Display.setCursor(0, 0);
    Display.setTextSize(2);
    Display.println("Radio");
    Display.setCursor(0, 30);
    Display.println(radioStationNames[currentStation]);
    Display.display();
    Serial.println("Radio");
    Serial.println(radioStationNames[currentStation]);
}

void displayClear() {
    Serial.println(" cleardisplay()");
    Display.clearDisplay();
    Serial.println(" display()");
    Display.display();
}

/*void menuControl() {
    switch (menu) {
        case TV_MENU:
        Menus.mainMenuText("TV", 30, 12, 5);
        lastMenuSet();
        break;
        case BLUETOOTH_MENU:
        Menus.bluetoothMenu("Bluetooth", "Connecting");
        break;
        case BLUETOOTH_CONNECTED:
        Menus.bluetoothMenu("Bluetooth", "Connected");
        break;
        case FM_MENU:
        Menus.radioMenu();
        lastMenuSet();
        break;
        case AUX_MENU:
        Menus.mainMenuText("AUX", 10, 12, 5);
        lastMenuSet();
        break;
        case VOLUME_MENU:
        Menus.volumeMenu();
        if (myLastTime == 0) {
        myLastTime = millis();
        }
        if ((millis() - myLastTime) > 1000) {
        myLastTime = 0;
        menu = lastMenu;
        }
        break;
    }
}
*/