#include "audio.h"
#include "WifiConfig.h"
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Disk/AudioSourceURL.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "BluetoothA2DPSink.h"

//Stream and quality
static AudioInfo info(48000, 2, 32); //48000 32 bit sample works the best with spdif to i2s converters even though the converter outputs 24 bit audio

static I2SStream i2sIn;
static I2SStream i2sOut;

//Bluetooth
static BluetoothA2DPSink a2dp_sink(i2sOut);

//Copying I2S stream from external SPDIF converter to external DAC. To actually run the stream you need to use copierInOut.copy() in loop()
static StreamCopy copierInOut(i2sOut, i2sIn);

//Internet Radio, don't use https becaus it will run out of memory. If https is the only option you can do it
static URLStream urlStream(WIFI, PASSWORD);
static AudioSourceURL urlSource(urlStream, urls, "audio/mp3");
static MP3DecoderHelix decoder;
static AudioPlayer player(urlSource, i2sOut, decoder);

uint8_t currentStation = 0; 

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


void i2sOutSetup() {
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
}

void detachAudio() {
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

void a2dpSinkStart() {
    a2dp_sink.start("MojAudio");
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