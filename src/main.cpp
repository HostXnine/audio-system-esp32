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
#include <IRremote.hpp>

/*To do list:
x make servo move for the non continouse one
x set RTC variable for storinf servo position between restarts
// set NVS variable for storing servos's position for accidental power offs
    // set that it won't store a new value into NVS if position didn't changed for X degrees
- set buttons on IR remotes
x adjust bluetooth text lower
x adjust vol/up/down text size
x add relay control for V5 DC supply
x add relay control for AC supply for the speakers
x add AUX input support
- add url radio connection timeout
x split the player contorls from the remote control funciton
x add ENUMS
- disable remote controll and other buttons when the power is off, especially for the volume buttons
- optimize the playerControl() function
- optimize the remoteControl() function
*/

// Taks and menu control global variables 
RTC_NOINIT_ATTR int task; //this is stored in memory which survies restarts but not power offs
int flag;
int menu;
int lastMenu;
esp_reset_reason_t resetReason = esp_reset_reason();
unsigned long myLastTime; //for implementing delays for some reason it has to be a global variable otherwise it doesn't work

// Relay setup
#define ON_OFF_5V_PIN 13
#define ON_OFF_AC_PIN 26
#define AUX_LEFT_PIN 25
#define AUX_RIGHT_PIN 33

// IR
#define IR_RECEIVE_PIN 19 // definition for IR
int irReceivedData; //Stores the decodded button presses

// Physical Buttons
#define BUTTON_A_PIN 27
#define BUTTON_B_PIN 14

// Serov
#define SERVO_PIN 23 //recommended pins 2 (if no led),4,12-19,21-23,25-27,32-33
RTC_NOINIT_ATTR int postitionRtc; // servo postition stored in ms
const int SERVO_MIN = 510;
const int SERVO_MAX = 2510;
int position = SERVO_MIN; // servo position stored in ms
int step = 10; // Used for the servo's position step
Servo myServo; // Create a "Servo" object called "servo"

//servo miliseconds position for continous SG90 servo is between 1490 - 1540 still. The speet range is min cca 900 and max cca 2100.
//The ideal stop pint is then 1515 milliseconds.
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

  //Copying I2S stream from external SPDIF converter to external DAC. 
  //This is only makes the object to actually run this you need to use copierInOut.copy() in void loop()
StreamCopy copierInOut(i2sOut, i2sIn);

  //Internet Radio
const char* URLS[] = {
  "http://live.radio.si/Toti",
  "http://reflector.radionet.si:8000/stream.ogg",
  "http://stream.srg-ssr.ch/m/drs3/mp3_128"
};

URLStream urlStream(WIFI, PASSWORD);
AudioSourceURL urlSource(urlStream, URLS, "audio/mp3");
MP3DecoderHelix decoder;
AudioPlayer player(urlSource, i2sOut, decoder);
Debouncer buttonDebouncer(2000); // for AudioPlayer

//OLED - i2c pins GPIO22 = SCK and GPIO21 = SDA
enum oledSettings {
  SCREEN_ADDRESS = 0x3C, ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
  SCREEN_WIDTH = 128,
  SCREEN_HEIGHT = 64,
  OLED_RESET = -1 // Reset pin # (or -1 if sharing Arduino reset pin)
};

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void remoteDecodeSignal() {
  if (IrReceiver.decode()) { //Returns true if anything is received by the remote
    //use all that has Serial in front for Decoding new remote:
    /* Serial.print("IrReceiver.decodedIRData.command=");
    Serial.print(IrReceiver.decodedIRData.command);
    Serial.print(" Protocol=");
    Serial.print(IrReceiver.decodedIRData.protocol);
    Serial.print(" ProtocolName=");
    Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol)); */
    irReceivedData = IrReceiver.decodedIRData.command; //stores IR decoded code in dec
    IrReceiver.resume();  //Receive the next value
  } 
  else {
    irReceivedData = 0xFFFF; //Just any big value. It could be 0 but since 0 is also for no signal it's easier to debug this way
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

class MenuClass {
  public:
  // static const unsigned char PROGMEM bluetooth_icon_64x64_bmp[4096];
  void mainMenuText(const char mainName[10]) {
    display.clearDisplay();
    display.setCursor(30, 12);
    display.setTextSize(5);
    display.println(mainName);
    display.display(); 
    Serial.println(mainName);
  }
  void mainMenuTextSmall(const char mainName[10]) {
    display.clearDisplay();
    display.setCursor(12, 30);
    display.setTextSize(2);
    display.println(mainName);
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
};

MenuClass Menus;

//This function checks if the servo is attached, if not it attaches it. Don't put attach servo in setup() because it makes a it jitter.
void isServoAttached() {
  if (myServo.attached() == false) { 
    myServo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
    position = postitionRtc;
  }
}

/* unsigned long  myLastTimeTwo;
bool attach = false;
void servoAttachDetach() {
  if (attach == true) {
    if (myLastTimeTwo == 0) {
        myLastTimeTwo = millis();
    }
      if ((millis() - myLastTimeTwo) >= 3000) {
        myLastTimeTwo = 0;
        attach = false;
        myServo.detach();
      }
    myServo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
    if (myLastTimeTwo == 0) {
        myLastTimeTwo = millis();
    }
      if ((millis() - myLastTimeTwo) >= 3000) {
        myLastTimeTwo = 0;
        attach = false;
        myServo.detach();
      }
  }
}
 */

enum remoteButtons {
  //Remote
  // const int volumeDown = 3; //- button wokwi simulator 152, real remote 3
  // const int volumeUp = 2; //+ button wokwi simulator 2, real remote 2
  // const int menuButton = 114; //task button wokwi simulator 226, real remote 114, key 5 = 53
  // const int nextButton = 142; //key 6 = 54, remote 142
  // const int prevButton = 143; //key 4 = 52, remote 143
  // const int stopButton = 177; // key 1 = 49, remote 177
  // const int pauseButton = 186; // key 3 = 51, remote 186
  // const int playButton = 176; //play, remote 176
  // const int power = 8;  //key 0 = 48 powe remote 8
  //zelen 113, rumen 99, plavi 97, mute 9, pgup 0, pgdown 1, 

  /* Buttons for Sharp remote:
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
  */

  //Genral (keyboard)
  VOLUME_UP = 43, // -
  VOLUME_DOWN = 45, // +
  MENU = 53, // 5
  POWER = 48, // 0
  DEBUG = 42, // *

  //player control (keyboard)
  NEXT = 54, // 6
  PREVIOUS = 52, // 4
  STOP = 49, // 1
  PAUSE = 51, // 3
  PLAY = 36 // home
};

enum menuAndTask {
  NO_OF_MAIN_MENU_ITEMS = 4,
  TV_MENU = 1,
  BLUETOOTH_MENU = 2,
  FM_MENU = 3,
  AUX_MENU = 4,
  VOLUME_MENU = 50,
  ON_OFF_MENU = 100
};

//Asings what buttons do. They also excecutes a processes in this function and are related totasks() function and menuControl() function
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
    if (task < ON_OFF_MENU) {
      task = ON_OFF_MENU;
    }
    else if (task == ON_OFF_MENU) {
      task = 1;
      irReceivedData = 0;
      restart();
    }
    break;
  }
}


void playerControl() {
  switch(irReceivedData) {
    case NEXT:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.next();
    } 
    else if (player.isActive()) {
      player.next();
    } 
    break;

    case PREVIOUS:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.previous();
    } 
    else if (player.isActive()) {
      player.previous();
    } 
    break;

    case STOP:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.stop();
    } 
    else if (player.isActive()) { 
      player.stop();
    } 
    break;

    case PAUSE:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.pause();
    } 
    break;

    case PLAY:
    if (a2dp_sink.is_connected()) {
      a2dp_sink.play();
    } 
    else if (player.isActive()) { 
      player.play();
    } 
    break;
  }
}

void lastMenuSet() {
  lastMenu = menu;
  menu = 0;
}

//This is only to defie the display function of the menu. The actual tasks or processes are in the task() funciton
void menuControl() {
  switch (menu) {
    case TV_MENU:
    Menus.mainMenuText("TV");
    lastMenuSet();
    break;

    case BLUETOOTH_MENU:
    Menus.mainMenuTextSmall("Bluetooth");
    lastMenuSet();
    break;

    case FM_MENU:
    Menus.mainMenuText("FM");
    lastMenuSet();
    break;

    case AUX_MENU:
    Menus.mainMenuText("AUX");
    lastMenuSet();
    break;

    case VOLUME_MENU:
    Menus.volumeMenu();
    if (myLastTime == 0) {
      myLastTime = millis();
    }
    if ((millis() - myLastTime) >= 1000) {
      myLastTime = 0;
      menu = lastMenu;
    }
    break;
  }
}

void flagTaskSet() {
  flag = task;
  menu = task;
}

//This are tasks related to actual connection to the audio
void tasks() {
  switch (task) {
    case TV_MENU:
    if (task != flag) { //inside this if statement is the "setup" code runs once
      flagTaskSet();
    }
    copierInOut.copy(); //here outside the above if statement is the "loop" code
    break;

    case BLUETOOTH_MENU:
    if (task != flag) {
      flagTaskSet();
      a2dp_sink.start("MojAudio");
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
      digitalWrite(ON_OFF_5V_PIN, LOW);
      digitalWrite(ON_OFF_AC_PIN, LOW);
      Serial.println("OFF");
    }
    break;
  }
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
  
  // Relays
  Serial.println("Relays setup");
  pinMode(ON_OFF_5V_PIN, OUTPUT);
  digitalWrite(ON_OFF_5V_PIN, HIGH);
  
  pinMode(ON_OFF_AC_PIN, OUTPUT);
  digitalWrite(ON_OFF_AC_PIN, HIGH);
  
  pinMode(AUX_LEFT_PIN, OUTPUT);
  digitalWrite(AUX_LEFT_PIN, LOW);
  
  pinMode(AUX_RIGHT_PIN, OUTPUT);
  digitalWrite(AUX_RIGHT_PIN, LOW);

  //IR
  Serial.println("IR setup");
  IrReceiver.begin(IR_RECEIVE_PIN);

  //Defining buttons by using internal pull-down resistors
  Serial.println("Buttons setup");
  pinMode(BUTTON_A_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON_B_PIN, INPUT_PULLDOWN);

  //Oled
  Serial.println("Oled Setup");
  display.begin(SCREEN_ADDRESS, true);
  display.setContrast (0);
  display.setTextColor(SH110X_WHITE);

  //I2S in
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

  // I2S out
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
  
  //Servo is attached in isServoAttached() function when needed

  Serial.println("Setup done");
}

void debug() {
  irReceivedData = Serial.read();
  if (irReceivedData > 0) {
    Serial.print("|Keypress: ");
    Serial.print(irReceivedData);
    Serial.print(" | task: ");
    Serial.print(task);
  }
}

void loop() {
  debug();
  //remoteDecodeSignal(); // comment it out for keyboard control
  remoteControl();
  playerControl();
  menuControl();
  tasks();
}