#include <Arduino.h>
#include "WifiConfig.h"
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Disk/AudioSourceURL.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "BluetoothA2DPSink.h"
#include <ESP32Servo.h>
#define DECODE_NEC
//#define DECODE_DENON //includes sharp. Has to be called before IRremote.hpp. Comment this when adding a new remote.
#include <IRremote.hpp>
#include "display.h"

/*To do list:
- add url radio connection timeout
- fix broken functionality when bluetooth is selected
- fix spageti code in tasks and menuControl
- optimize deoubce for remote control
- maybe split the remote and remote debounce functionality since for some buttons it has to be set at a different timeout
*/

// Tasks and menu control global variables
esp_reset_reason_t resetReason = esp_reset_reason();
uint32_t myLastTime; //for implementing delays. For some reason it has to be a global variable otherwise it doesn't work.

// Relays
#define ON_OFF_5V_PIN 13
#define ON_OFF_AC_PIN 26
#define AUX_LEFT_PIN 25
#define AUX_RIGHT_PIN 33

// IR
#define IR_RECEIVE_PIN 19
unsigned long lastDebounceTime = 0;

// Physical Buttons
#define BUTTON_A_PIN 27
#define BUTTON_B_PIN 14

// Serov
#define SERVO_PIN 23 //recommended pins 2 (if no led),4,12-19,21-23,25-27,32-33
RTC_NOINIT_ATTR uint16_t postitionRtc; // servo postition stored in ms
constexpr uint16_t SERVO_MIN = 510;
constexpr uint16_t SERVO_MAX = 2510;
uint16_t position = SERVO_MIN; // servo position stored in ms
uint16_t step = 10; // Servo's one step movement in ms
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
const char* urls[3] = { 
  "http://live.radio.si/Toti",
  "http://reflector.radionet.si:8000/stream.ogg",
  "http://livestreaming-node-3.srg-ssr.ch/srgssr/srf3/mp3/128"
};

const char* radioStationNames[3] = {
  "Toti Radio",
  "NET FM",
  "Swiss"
};
uint8_t currentStation = 0;

URLStream urlStream(WIFI, PASSWORD);
AudioSourceURL urlSource(urlStream, urls, "audio/mp3");
MP3DecoderHelix decoder;
AudioPlayer player(urlSource, i2sOut, decoder);
//Debouncer buttonDebouncer(); // for AudioPlayer

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

enum SystemState {
  TV_STATE,
  BLUETOOTH_STATE,
  RADIO_STATE,
  AUX_STATE,
  OFF_STATE, //keep it always before the last item
  STATE_COUNT // keep it always last
};

RTC_NOINIT_ATTR enum SystemState currentSystemState = TV_STATE;

enum Buttons {
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
  MENU_BUTTON = 5,
  POWER_BUTTON = 20,
  VOLUME_UP_BUTTON = 24,
  VOLUME_DOWN_BUTTON = 25,
  DEBUG_BUTTON = 500,
  //Player control (NEC)
  NEXT_BUTTON = 3,
  PREVIOUS_BUTTON = 2,
  STOP_BUTTON = 1,
  PAUSE_BUTTON = 11,
  PLAY_BUTTON = 8,
  NONE_BUTTON = 0,
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

Buttons currentButton = NONE_BUTTON;

void decodeNewRemote() { //only used when decoding a new remote
  if (IrReceiver.decode()) {
    Serial.print("IrReceiver.decodedIRData.command=");
    Serial.print(IrReceiver.decodedIRData.command);
    Serial.print(" Protocol=");
    Serial.print(IrReceiver.decodedIRData.protocol);
    Serial.print(" ProtocolName=");
    Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));
    IrReceiver.resume();
  }
}

void detachAll() {
Serial.println(" Writing servo position to RTC... ");
  postitionRtc = position;
  Serial.println(" Detaching...");
  myServo.detach();
  Serial.println(" cleardisplay()");
  //display.clearDisplay();
  Serial.println(" display()");
  //display.display();
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

void restartSystem() {
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



/*void playerControl() {
  int8_t sizeOfUrls = (sizeof(urls) / sizeof(urls[0]));

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
*/

enum IrState {
  IR_RECEIVE_STATE,
  IR_DEBOUNCE_STATE,
  IR_RESET_STATE,
  STATE_IR_COUNT
};

unsigned long debounceDelay;
bool initIrDebounce = false;
IrState currentIrState = IR_RECEIVE_STATE;



void changeIrState();
void debounceIr(unsigned long debounceDelay = 500) {
  if (!initIrDebounce) {
    initIrDebounce = true;
    lastDebounceTime = millis();
    Serial.println(" Debounce start ");
  }
  if (((millis() - lastDebounceTime) > (debounceDelay)) && (initIrDebounce)) {
    initIrDebounce = false;
    Serial.println("Debounce ends");
    changeIrState(); //changes state!!!
  }
}

bool validButton(Buttons button) {
  switch(button) {
    case MENU_BUTTON:
      Serial.println("validButton(MENU_BUTTON)");
      return true;
      break;
    case POWER_BUTTON:
      Serial.println("validButton(POWER_BUTTON)");
      return true;
      break;
    default:
      Serial.println("validButton(default)");
      return false;
      break;
  }
}

void changeIrState() {
  switch(currentIrState) {
    case IR_RECEIVE_STATE: {
      bool valid = validButton(currentButton);
      if (valid) {
        Serial.println("changeIrState = IR_DEBOUNCE_STATE");
        currentIrState = IR_DEBOUNCE_STATE;
      } else { 
      Serial.println("changeIrState = IR_RESET_STATE");
       currentIrState = IR_RESET_STATE;
      }
    break;
    }
    case IR_DEBOUNCE_STATE:
      if (!initIrDebounce) {
        Serial.println("changeIrState = IR_RECEIVE_STATE");
        currentIrState = IR_RESET_STATE;
      }
      break;
    case IR_RESET_STATE:
      currentButton = NONE_BUTTON;
      currentIrState = IR_RECEIVE_STATE;
      break;
  }
}

void buttonInput(Buttons &button);

void irStateLoop(IrState state) {
  switch (state) {
    case IR_RECEIVE_STATE:
      Serial.println("IR_RECEIVE_STATE");
      if (IrReceiver.decode()) {
        currentButton = (static_cast<Buttons>(IrReceiver.decodedIRData.command));
        buttonInput(currentButton);
      }
      break;
    case IR_DEBOUNCE_STATE:
      Serial.println("IR_DEBOUNCE_STATE");
      debounceIr(debounceDelay); //self sets to IR_RESET_STATE
      break;
    case IR_RESET_STATE:
      Serial.println("IR_RESET_STATE");
      IrReceiver.resume();
      changeIrState(); //self sets to IR_RECEIVE_STATE
      break;
  }
}

void systemStateSetup(SystemState state) { //has to run only once
  switch (state) {
    case TV_STATE:
    Serial.println("TV_STATE SETUP");
    break;
    case BLUETOOTH_STATE:
    Serial.println("BLUETOOTH_STATE SETUP");
    a2dp_sink.start("MojAudio");
    break;
    case RADIO_STATE:
    Serial.println("RADIO_STATE SETUP");
    player.begin();
    break;
    case AUX_STATE:
    Serial.println("AUX_STATE SETUP");
    if (digitalRead(AUX_LEFT_PIN) == LOW || digitalRead(AUX_RIGHT_PIN) == LOW) {
      digitalWrite(AUX_LEFT_PIN, HIGH);
      digitalWrite(AUX_RIGHT_PIN, HIGH);
    }
    break;
    case OFF_STATE:
    Serial.println("OFF_STATE SETUP");
    detachAll();
    digitalWrite(AUX_LEFT_PIN, LOW);
    digitalWrite(AUX_RIGHT_PIN, LOW);
    digitalWrite(ON_OFF_5V_PIN, LOW);
    digitalWrite(ON_OFF_AC_PIN, LOW);
    Serial.println("OFF  ");
    break;
  }
}

void systemStateLoop(SystemState state) {
  switch (state) {
    case TV_STATE:
    Serial.println("TV_STATE LOOP");
    copierInOut.copy();
    break;
    case RADIO_STATE:
    Serial.println("RADIO_STATE LOOP");
    player.copy();
    break;
    default:
    break;
  }
}

void setSystemState (SystemState newState) {
  if (newState == OFF_STATE) {
    Serial.println("setSystemState == OFF_STATE");
    currentSystemState = OFF_STATE;
    systemStateSetup(currentSystemState);
    return;
  }  
  if (newState != currentSystemState) {
    Serial.println("setSystemState has changed");
    currentSystemState = newState;
    restartSystem();
  } else {
    Serial.println("setSystemState runs systemStateSetup");
    systemStateSetup(currentSystemState);
  }
}

void changeSystemState() { //rotates through system states
  if (currentSystemState != AUX_STATE) {
    Serial.println("changeSystemState != AUX_STATE");
    setSystemState(static_cast<SystemState>((currentSystemState + 1) % STATE_COUNT));
  } else {
    Serial.println("changeSystemState == AUX_STATE");
    setSystemState(static_cast<SystemState>((currentSystemState + 2) % STATE_COUNT));
  }
}

void changeOnOffSystemState() {
  if (currentSystemState != OFF_STATE){
    Serial.println("changeOnOffSystemState() != OFF_STATE");
    setSystemState(OFF_STATE);
  } else {
    Serial.println("changeOnOffSystemState() == OFF_STATE");
    setSystemState(static_cast<SystemState>(STATE_COUNT % STATE_COUNT)); //with other words set to 0
  }
}

void buttonInput(Buttons &button) {
  switch(button) { //button press should be run only once!
    case MENU_BUTTON:
      Serial.println("button = MENUT_BUTTON");
      debounceDelay = 1000; //if don't set this then the default value is 500 ms
      changeIrState();
      changeSystemState();
      break;
    case POWER_BUTTON:
      Serial.println("button = POWER_BUTTON");
      changeIrState();
      changeOnOffSystemState();
      break;
    default:
      Serial.println("button = non-valid button");
      changeIrState();
      break;
  }
  Serial.println("button = NONE_BUTTON");
  button = NONE_BUTTON;
}

void isServoAttached() {
  Serial.println("isServoAttached");
  if (myServo.attached() == false) { 
    myServo.attach(SERVO_PIN, SERVO_MIN, SERVO_MAX);
    position = postitionRtc;
  }
}

void setup() {
  //Debugging
  Serial.begin(115200);
  Serial.println("Setup started");
  
  //System state setup  
  if (resetReason != ESP_RST_SW) {
    Serial.println("RTC variabke setup");
    setSystemState(static_cast<SystemState>(STATE_COUNT % STATE_COUNT)); //with other words set to 0
    Serial.println("setup() setSystemState(0)");
  } else {
    setSystemState(currentSystemState);
    Serial.println("setup() setSystemState(currentSystemState)");
  }
    
//RTC variable
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

  i2sInSetup();

  Serial.println("Setup done");
}

void debug() {
  //irReceivedData = Serial.read(); // uncomment for keyboard control
  if (currentButton > 0) {
    Serial.print("|Keypress: ");
    Serial.print(currentButton);
  }
}

void loop() {
  //decodeNewRemote(); //uncomment this for decoding a new remote
  irStateLoop(currentIrState);
  systemStateLoop(currentSystemState);
  //debug();
}