#include "audio.h"
#include "wifi_config.h"
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Disk/AudioSourceURL.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "BluetoothA2DPSink.h"
#include <Arduino.h>
#include "debug.h"

//Audio OUT (external DAC)
constexpr uint8_t OUT_I2S_BCK_PIN = 17; //aka SCK. Connect to (BCK)
constexpr uint8_t OUT_I2S_DATA_PIN = 16; //Connect to (DIN)
constexpr uint8_t OUT_I2S_WS_PIN = 4; //aka LRCLK. Connect to (LCK)
// SCK connect to GND

//Aduio IN (external SPDIF to I2S converer)
constexpr uint8_t IN_I2S_WS_PIN = 34; //aka LRCLK. Connect to (LRCK)
constexpr uint8_t IN_I2S_BCK_PIN = 35; //aka SCK. Connect to (BCKL)
constexpr uint8_t IN_I2S_DATA_PIN = 32; // data out. Connect to (DATA)
//constexpr uint8_t IN_I2S_MCK_PIN = 3;  //must be 0,1 or 3. NEVER USE 1 it's the TX pin. 3 is the RX pin and 0 is not exposed. The external DAC works without MCK connection.

//Stream and quality
static AudioInfo info(48000, 2, 32); //48000 32 bit sample works the best with spdif to i2s converters even though the converter outputs 24 bit audio

static I2SStream i2sIn;
static I2SStream i2sOut;

//Bluetooth
BluetoothA2DPSink a2dp_sink(i2sOut);

//Copying I2S stream from external SPDIF converter to external DAC. To actually run the stream you need to use copierInOut.copy() in loop()
static StreamCopy copierInOut(i2sOut, i2sIn);

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

static uint8_t currentStation = 0;
static int8_t sizeOfUrls = (sizeof(urls) / sizeof(urls[0]));
const char* currentRadioStationName = radioStationNames[currentStation];

//Internet Radio, don't use https becaus it will run out of memory. If https is the only option you can do it
static URLStream urlStream(WIFI, PASSWORD);
static AudioSourceURL urlSource(urlStream, urls, "audio/mp3");
static MP3DecoderHelix decoder;
static AudioPlayer player(urlSource, i2sOut, decoder);
//Debouncer buttonDebouncer(); // for AudioPlayer

//Debouncer buttonDebouncer(); // for AudioPlayer

void i2sInSetup() {
    debugln("I2S in setup");
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

void i2sOutSetup() {
    debugln("I2S out setup"); 
    auto cfgOut = i2sOut.defaultConfig(TX_MODE);
    cfgOut.copyFrom(info); 
    cfgOut.i2s_format = I2S_PHILIPS_FORMAT;
    cfgOut.is_master = true;
    cfgOut.port_no = 1;
    cfgOut.pin_ws = OUT_I2S_WS_PIN;
    cfgOut.pin_bck = OUT_I2S_BCK_PIN;
    cfgOut.pin_data = OUT_I2S_DATA_PIN;
    i2sOut.begin(cfgOut);
}

void detachAudio() {
    if (player.isActive()) {
        debugln(" player.end");
        player.end();
    }
    debugln(" a2dp_sink.disconnect");
    if (a2dp_sink.is_connected()) {
        a2dp_sink.disconnect();
        debugln(" a2dp_sink.end");
        a2dp_sink.end();
        delay(50);
        debugln(" btStop");
        btStop();
    }
    debugln(" i2sOut.End");
    i2sOut.end();
    debugln(" i2sIn.end");
    i2sIn.end();
}

void a2dpStart() {
    a2dp_sink.start("MojAudio");
}
bool a2dpIsConnected() {
    return a2dp_sink.is_connected();
}

void playerBegin() {
    player.begin();
}

void copierInOutCopy() {
    copierInOut.copy();
}

void playerCopy() {
    player.copy();
}

void audioNext() {
    if (a2dp_sink.is_connected()) {
        a2dp_sink.next();
    } 
    else if (player.isActive()) {
        player.next();
        currentStation = ((currentStation + 1) % sizeOfUrls);
        currentRadioStationName = radioStationNames[currentStation];
    } 
}

void audioPrevious() {
    if (a2dp_sink.is_connected()) {
        a2dp_sink.previous();
    } 
    else if (player.isActive()) {
        player.previous();
        currentStation = ((currentStation - 1 + sizeOfUrls) % sizeOfUrls);
        currentRadioStationName = radioStationNames[currentStation];
    }
}

void audioPlayPause() {
    static bool paused = false;
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
}