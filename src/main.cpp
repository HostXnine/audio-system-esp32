#include <Arduino.h>
#include "WifiConfig.h"
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Disk/AudioSourceURL.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "BluetoothA2DPSink.h"
#include <ESP32Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#define DECODE_NEC
//#define DECODE_DENON //includes sharp. Has to be called before IRremote.hpp. Comment this when adding a new remote.
#include <IRremote.hpp>

/*To do list:
x make servo move for the non continouse one
x set RTC variable for storinf servo position between restarts
// set NVS variable for storing servos's position for accidental power offs
    // set that it won't store a new value into NVS if position didn't changed for X degrees
x set buttons on IR remotes
x adjust bluetooth text lower
x adjust vol/up/down text size
x add relay control for V5 DC supply
x add relay control for AC supply for the speakers
x add AUX input support
x add url radio station to the oled display
x split the player contorls from the remote control funciton
x add ENUMS
x disable remote controll and other buttons when the power is off, especially for the volume buttons
x optimize the playerControl() function
x optimize the remoteControl() function
x add debaunce for remote control
- add url radio connection timeout
? fix the power button isue
- optimize deoubce for remote control
- maybe split the remote and remote debounce functionality since for some buttons it has to be set at a different timeout
x add connecting and connected to the display when using bluetooth
x optimize the setup() code
*/

// Tasks and menu control global variables
RTC_NOINIT_ATTR int task; //RTC variable survive restarts
int flag;
int menu;
int lastMenu;
esp_reset_reason_t resetReason = esp_reset_reason();
unsigned long myLastTime; //for implementing delays. For some reason it has to be a global variable otherwise it doesn't work.

// Relays
#define ON_OFF_5V_PIN 13
#define ON_OFF_AC_PIN 26
#define AUX_LEFT_PIN 25
#define AUX_RIGHT_PIN 33

// IR
#define IR_RECEIVE_PIN 19
int irReceivedData; //Stores the decodded button presses
bool irState = false;
unsigned long lastDebounceTime = 0;

// Physical Buttons
#define BUTTON_A_PIN 27
#define BUTTON_B_PIN 14

// Serov
#define SERVO_PIN 23 //recommended pins 2 (if no led),4,12-19,21-23,25-27,32-33
RTC_NOINIT_ATTR int postitionRtc; // servo postition stored in ms
const int SERVO_MIN = 510;
const int SERVO_MAX = 2510;
int position = SERVO_MIN; // servo position stored in ms
int step = 10; // Servo's one step movement in ms
Servo myServo;

//Servo is still at 1490 - 1540 if you use a 360 (SG90) servo. Speed min 900 - max 2100. Ideal stop pint is 1515 ms.
//I have set the forward and backward motion speed to 100 milliseconds difference from the stop point.

//Audio OUT (external DAC)
#define OUT_I2S_BCK_PIN 17 //aka SCK. Connect to (BCK)
#define OUT_I2S_DATA_PIN 16 //Connect to (DIN)
#define OUT_I2S_WS_PIN 4 //aka LRCLK. Connect to (LCK)
// SCK connect to GND

//Aduio IN (external SPDIF to I2S converer)
#define IN_I2S_WS_PIN 34 //aka LRCLK. Connect to (LRCK)
#define IN_I2S_DATA_PIN 35 // data out. Connect to (DATA)
#define IN_I2S_BCK_PIN 32 //aka SCK. Connect to (BCKL)
//#define IN_I2S_MCK_PIN 3  //must be 0,1 or 3. NEVER USE 1 it's the TX pin. 3 is the RX pin and 0 is not exposed. The external DAC works without MCK connection.

//Stream and quality
AudioInfo info(48000, 2, 32); //48000 32 bit sample works the best with spdif to i2s converters even though the converter outputs 24 bit audio
I2SStream i2sIn;
I2SStream i2sOut;

//Bluetooth
BluetoothA2DPSink a2dp_sink(i2sOut);

//Copying I2S stream from external SPDIF converter to external DAC. To actually run the stream you need to use copierInOut.copy() in loop()
StreamCopy copierInOut(i2sOut, i2sIn);

//Internet Radio, don't use https becaus it will run out of memory. If https is the only option you can do it
const char* URLS[] = { 
  "http://live.radio.si/Toti",
  "http://reflector.radionet.si:8000/stream.ogg",
  "http://livestreaming-node-3.srg-ssr.ch/srgssr/srf3/mp3/128"
};

const char* RADIO_STATION_NAMES[] = {
  "Toti Radio",
  "NET FM",
  "Swiss Radi"
};
int currentStation = 0;

URLStream urlStream(WIFI, PASSWORD);
AudioSourceURL urlSource(urlStream, URLS, "audio/mp3");
MP3DecoderHelix decoder;
AudioPlayer player(urlSource, i2sOut, decoder);
//Debouncer buttonDebouncer(); // for AudioPlayer

//OLED - i2c pins GPIO22 = SCK and GPIO21 = SDA
enum oledSettings {
  SCREEN_ADDRESS = 0x3C, ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
  SCREEN_WIDTH = 128,
  SCREEN_HEIGHT = 64,
  OLED_RESET = -1 // Reset pin # (or -1 if sharing Arduino reset pin)
};

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum menuAndTask {
  NO_OF_MAIN_MENU_ITEMS = 4, //update this number if you add a main menu item
  TV_MENU = 1,
  BLUETOOTH_MENU = 2,
  FM_MENU = 3,
  AUX_MENU = 4,
  VOLUME_MENU = 50,
  BLUETOOTH_CONNECTED = 51,
  ON_OFF_MENU = 100
};

enum remoteButtons {
  /* Buttons for Sharp and NEC
  Buttons for Sharp remote it uses DECODE_DENON:
  NET 149      |   ON/OFF 233
     1 254 | 2 253 |   3 252
     4 251 | 5 250 |   6 249
     7 248 | 8 247 |   9 246
  <--> 216 | 0 254 |->[] 236
  <> 55 | v/oo 231 | i+ 228 | FAV 86
  vol+ 235  | 3D 76  | P^ 238
  vol- 234  | EPG 92 | Pv 237 
  mute 232 | MODE/AV 7 | ECO 200 | END 10
           ^ 168
     < 40 |OK 173| > 39
           v 223
  MENU 57            backArrow 27              
       red 183 |    green 182 |   yellow 181 |    blue 180
  teletext 203 | subtitles 96 |   ATV/DTV 95 |    RADIO 91
    SOURCE 255 |      REC 250 | REC STOP 249 | USB REC 194
        << 252 |       [] 253 |     >/II 254 |      >> 251 

  Buttons on VCR remote. I uses DECODE_NEC
  Operate 20
  1  5 | 2  6 | 3  7
  4 12 | 5 13 | 6 14
  7 15 | 8 28 | 9 29
  v 25 | 0  4 | ^ 24
  << 2 | >  8 | >> 3   | [] inactive
  ||11 | [] 1 | ||> 16 | O  inactive
  When pressed [] and O simultanious it is 9
  */
  //General (NEC)
  MENU = 5,
  POWER = 20,
  VOLUME_UP = 24,
  VOLUME_DOWN = 25,
  DEBUG = 500,
  //Player control (NEC)
  NEXT = 3,
  PREVIOUS = 2,
  STOP = 1,
  PAUSE = 11,
  PLAY = 8,
 /*  //General (keyboard)
  MENU = 53, // 5
  POWER = 48, // 0
  VOLUME_UP = 43, // -
  VOLUME_DOWN = 45, // +
  DEBUG = 42, // *
  //Player control (keyboard)
  NEXT = 54, // 6
  PREVIOUS = 52, // 4
  STOP = 49, // 1
  PAUSE = 51, // 3
  PLAY = 36 // home */
};

void decodeNewRemote() { //only used when decoding a new remote
  if (IrReceiver.decode()) {
    Serial.print("IrReceiver.decodedIRData.command=");
    Serial.print(IrReceiver.decodedIRData.command);
    Serial.print(" Protocol=");
    Serial.print(IrReceiver.decodedIRData.protocol);
    Serial.print(" ProtocolName=");
    Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));
    irReceivedData = IrReceiver.decodedIRData.command;
    IrReceiver.resume();
  } 
  else {
    irReceivedData = 0;
    IrReceiver.resume();
  } 
}

void remoteDebounceOnly(uint16_t &button, const unsigned long DEBOUNCE_DELAY_IR) {
  irReceivedData = 0;
  if (IrReceiver.decode() && (!irState)) {
    lastDebounceTime = millis();
    irState = true;
    Serial.println(" PRESSED ");
    irReceivedData = IrReceiver.decodedIRData.command;
    Serial.print(" irReceivedData = IrReceiver.decodedIRData.command = ");
    Serial.println(irReceivedData);
  }
  if (((millis() - lastDebounceTime) > DEBOUNCE_DELAY_IR) && (irState)) {
    irState = false;
    IrReceiver.resume();
    Serial.print(" IrReceiver.resume() =  ");
    Serial.println(irReceivedData);
  }
}

void remoteDecodeSignal() {
  const unsigned long DEBOUNCE_DELAY_IR = 500;
  irReceivedData = 0;
  if (IrReceiver.decode() && (!irState)) {
    lastDebounceTime = millis();
    irState = true;
    Serial.println(" PRESSED ");
    irReceivedData = IrReceiver.decodedIRData.command;
    Serial.print(" irReceivedData = IrReceiver.decodedIRData.command = ");
    Serial.println(irReceivedData);
  }
  if (((millis() - lastDebounceTime) > DEBOUNCE_DELAY_IR) && (irState)) {
    irState = false;
    IrReceiver.resume();
    Serial.print(" IrReceiver.resume() =  ");
    Serial.println(irReceivedData);
  }
}

void remoteOnOff() {
  if (task != ON_OFF_MENU) {
    remoteDecodeSignal();
    return;
  }
  if ((task == ON_OFF_MENU) && (irState))  {
    irState = false;
    IrReceiver.resume();
  }
  if (!IrReceiver.decode()) {
    return;
  }
  if (IrReceiver.decodedIRData.command == POWER && !irState) {
    irReceivedData = POWER;
  } else {
    IrReceiver.resume();
  }
}

void detachAll() {
Serial.println(" Writing servo position to RTC... ");
  postitionRtc = position;
  Serial.println(" Detaching...");
  myServo.detach();
  Serial.println(" cleardisplay()");
  display.clearDisplay();
  Serial.println(" display()");
  display.display();
  if (player.isActive()) {
    Serial.println(" player.end");
    player.end();
  }
  Serial.println(" a2dp_sink.disconnect");
  if (a2dp_sink.is_connected()) {
    a2dp_sink.disconnect();
    Serial.println(" a2dp_sink.end");
    a2dp_sink.end();
    delay(50);
    Serial.println(" btStop");
    btStop();
  }
  Serial.println(" i2sOut.End");
  i2sOut.end();
  Serial.println(" i2sIn.end");
  i2sIn.end();
}

void restart() {
  Serial.println(" digitalWrite ON_OFF_PIN");
  if (digitalRead(ON_OFF_5V_PIN) == LOW) {
    digitalWrite(ON_OFF_5V_PIN, HIGH);
  }
  if (digitalRead(ON_OFF_AC_PIN) == LOW) {
    digitalWrite(ON_OFF_AC_PIN, HIGH);
  }
  detachAll();
  Serial.println(" Restarting now...");
  esp_restart();
}

//Checks if servo is attached, if not it attaches it. Don't put servo attach in setup() because it makes it jitter.
void isServoAttached();

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
    display.println(position);
    display.display();
    Serial.println(position);
  }
  void radioMenu() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println("Radio");
    display.setCursor(0, 30);
    display.println(RADIO_STATION_NAMES[currentStation]);
    display.display();
    Serial.println("Radio");
    Serial.println(RADIO_STATION_NAMES[currentStation]);
  }
};

MenuClass Menus;

//Asings what buttons do. They are also related to tasks() and menuControl() functionality

void remoteControl() {

  int buttonAState = digitalRead(BUTTON_A_PIN);
  int buttonBState = digitalRead(BUTTON_B_PIN);

  if (buttonAState == HIGH) {
    irReceivedData = VOLUME_UP;
  }
  if (buttonBState == HIGH) {
    irReceivedData = VOLUME_DOWN;
  }

  switch(irReceivedData) {
    case DEBUG:
    task = 1;
    break;
    case VOLUME_UP:
    menu = VOLUME_MENU;
    isServoAttached();
    if (position<=SERVO_MAX) {
      position += step;
      myServo.writeMicroseconds(position);
    } 
    break;
    case VOLUME_DOWN:
    menu = VOLUME_MENU;
    isServoAttached();
    if (position>=SERVO_MIN) {
      position -= step;
      myServo.writeMicroseconds(position);
    }
    break;
    case MENU: 
    if ((task < NO_OF_MAIN_MENU_ITEMS) && (task >= 0)) {
      task++;
      restart();
    } 
    else {
      task = 1;
      restart();
    }
    break;
    case POWER:
    irReceivedData = 0;
    if (task != ON_OFF_MENU) {
      task = ON_OFF_MENU;
      return;
    }
    if (task == ON_OFF_MENU) {
      task = TV_MENU;
      restart();
    }
    break;
  }
}

bool paused = false;

void playerControl() {
  int sizeOfUrls = (sizeof(URLS) / sizeof(URLS[0]));

  switch(irReceivedData) {
    case NEXT:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.next();
    } 
    else if (player.isActive()) {
      player.next();
      currentStation = ((currentStation + 1) % sizeOfUrls);
      Menus.radioMenu();
    } 
    break;
    case PREVIOUS:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.previous();
    } 
    else if (player.isActive()) {
      player.previous();
      currentStation = ((currentStation - 1 + sizeOfUrls) % sizeOfUrls);
      Menus.radioMenu();
    }
    break;
    case PLAY:
    if (a2dp_sink.is_connected()) {
      if (!paused) {
        a2dp_sink.pause();
        paused = true;
      }
      if (paused) {
        a2dp_sink.play();
        paused = false;
      }
    } 
    break;
  }
}

// Only to define menu control as seen on the oled screen.
void lastMenuSet() {
  lastMenu = menu;
  menu = 0;
}

void menuControl() {
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

//Tasks
void flagTaskSet() {
  flag = task;
  menu = task;
}

bool connected = false;
void i2sInSetup();
void tasks() {
  switch (task) {
    case TV_MENU:
    if (task != flag) { //inside this if is the "setup" code, it runs once
      i2sInSetup();
      flagTaskSet();
    }
    copierInOut.copy(); // outside the above if is the "loop" code
    break;
    case BLUETOOTH_MENU:
    if (task != flag) {
      flag = task;
      a2dp_sink.start("MojAudio");
    }
    if (!a2dp_sink.is_connected() && (!connected)) {
      menu = BLUETOOTH_MENU;
      connected = true;
    }
    if (a2dp_sink.is_connected() && (connected)) {
      menu = BLUETOOTH_CONNECTED;
      connected = false;
    }
    break;
    case FM_MENU:
    if (task != flag) {
      flagTaskSet();
      player.begin();
      //AudioLogger::instance().begin(Serial, AudioLogger::Info);//for debbuging
    }     
    player.copy();
    break;
    case AUX_MENU:
    if (task != flag) {
      flagTaskSet();
      if (digitalRead(AUX_LEFT_PIN) == LOW || digitalRead(AUX_RIGHT_PIN) == LOW) {
        digitalWrite(AUX_LEFT_PIN, HIGH);
        digitalWrite(AUX_RIGHT_PIN, HIGH);
      }
    }
    break;
    case ON_OFF_MENU:
    if (task != flag) {
      flag = task;
      detachAll();
      digitalWrite(AUX_LEFT_PIN, LOW);
      digitalWrite(AUX_RIGHT_PIN, LOW);
      digitalWrite(ON_OFF_5V_PIN, LOW);
      digitalWrite(ON_OFF_AC_PIN, LOW);
      Serial.print("OFF  ");
      Serial.println(irState);
    }
    break;
  }
}

void isServoAttached() {
  Serial.println("isServoAttached");
  if (myServo.attached() == false) { 
    myServo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
    position = postitionRtc;
  }
}

void i2sInSetup() {
  Serial.println("I2S in setup");
  auto cfgIn = i2sIn.defaultConfig(RX_MODE);
  cfgIn.copyFrom(info);
  cfgIn.i2s_format = I2S_PHILIPS_FORMAT;
  cfgIn.is_master = false;
  cfgIn.port_no = 0;
  cfgIn.pin_ws = IN_I2S_WS_PIN;
  cfgIn.pin_bck = IN_I2S_BCK_PIN;
  cfgIn.pin_data = IN_I2S_DATA_PIN;
  i2sIn.begin(cfgIn);
}

void setup() {
  //Debugging
  Serial.begin(115200);
  Serial.println("Setup started");
  
  //RTC variable
  if (resetReason != ESP_RST_SW) {
    Serial.println("RTC variabke setup");
    task = 1;
  }
  if ((postitionRtc < SERVO_MIN-100) || (postitionRtc > SERVO_MAX+100)) {
    Serial.println("calibrating positionRTC setup");
    postitionRtc = SERVO_MIN+500;
  }

  //Relays
  Serial.println("Relays setup");
  pinMode(ON_OFF_5V_PIN, OUTPUT);
  digitalWrite(ON_OFF_5V_PIN, HIGH);
  pinMode(ON_OFF_AC_PIN, OUTPUT);
  digitalWrite(ON_OFF_AC_PIN, HIGH);
  Serial.println("AUX setup");
  pinMode(AUX_LEFT_PIN, OUTPUT);
  digitalWrite(AUX_LEFT_PIN, LOW); //when LOW then it plays ADC IN
  pinMode(AUX_RIGHT_PIN, OUTPUT);
  digitalWrite(AUX_RIGHT_PIN, LOW);

  //IR
  Serial.println("IR setup");
  IrReceiver.begin(IR_RECEIVE_PIN);

  //Buttons
  Serial.println("Buttons setup");
  pinMode(BUTTON_A_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON_B_PIN, INPUT_PULLDOWN);

  //Oled
  Serial.println("Oled Setup");
  display.begin(SCREEN_ADDRESS, true);
  display.setContrast (0);
  display.setTextColor(SH110X_WHITE);

  //I2S out
  Serial.println("I2S out setup"); 
  auto cfgOut = i2sOut.defaultConfig(TX_MODE);
  cfgOut.copyFrom(info); 
  cfgOut.i2s_format = I2S_PHILIPS_FORMAT;
  cfgOut.is_master = true;
  cfgOut.port_no = 1;
  cfgOut.pin_ws = OUT_I2S_WS_PIN;
  cfgOut.pin_bck = OUT_I2S_BCK_PIN;
  cfgOut.pin_data = OUT_I2S_DATA_PIN;
  i2sOut.begin(cfgOut);

  Serial.println("Setup done");
}

void debug() {
  //irReceivedData = Serial.read(); // uncomment for keyboard control
  if (irReceivedData > 0) {
    Serial.print("|Keypress: ");
    Serial.print(irReceivedData);
    Serial.print(" | task: ");
    Serial.print(task);
  }
}

void loop() {
  //decodeNewRemote(); //uncomment this and comment out remoteOnOff() decode a new remote
  remoteOnOff(); // comment it out for keyboard control
  remoteControl();
  playerControl();
  menuControl();
  tasks();
  debug();
}