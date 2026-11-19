#include "audio.h"
#include "wifi_config.h"
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Disk/AudioSourceURL.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "BluetoothA2DPSink.h"
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
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
static BluetoothA2DPSink a2dp_sink(i2sOut);

//Copying I2S stream from external SPDIF converter to external DAC. To actually run the stream you need to use copierInOut.copy() in loop()
static StreamCopy copierInOut(i2sOut, i2sIn);

const char* urls[3] = { 
    "http://live.radio.si/Toti",
    "http://reflector.radionet.si:8000/stream.ogg.m3u",
    "http://livestreaming-node-3.srg-ssr.ch/srgssr/srf3/mp3/128"
};

const char* radioStationNames[3] = {
    "Toti Radio",
    "NET FM",
    "Swiss"
};

static volatile uint8_t currentStation = 0; // written by loop(), read by the radio task
static int8_t sizeOfUrls = (sizeof(urls) / sizeof(urls[0]));
const char* currentRadioStationName = radioStationNames[currentStation];

//Internet Radio, don't use https becaus it will run out of memory. If https is the only option you can do it
// No WiFi credentials here on purpose: URLStream's own login() waits for WiFi FOREVER.
// WiFi is connected by the radio task below, with a timeout.
static URLStream urlStream;
static AudioSourceURL urlSource(urlStream, urls, "audio/mp3");
static MP3DecoderHelix decoder;
static AudioPlayer player(urlSource, i2sOut, decoder);

// ---- Radio task ----
// All network work (WiFi connect, HTTP connect, reading the stream) can block for seconds,
// so it runs in its own task. loop() stays free to react to the remote at any time.
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000; // give up on WiFi after this
static constexpr int      URL_TIMEOUT_MS          = 5000;  // HTTP connect / first data (library default: 60 s)
static constexpr int      NO_DATA_TIMEOUT_MS      = 8000;  // stream delivers nothing -> treat as failed
static constexpr uint32_t RETRY_DELAY_MS          = 5000;  // wait before retrying WiFi / station

enum class RadioCmd : uint8_t { NONE, SELECT, STOP };
static std::atomic<uint8_t> radioCmd{static_cast<uint8_t>(RadioCmd::NONE)};
static std::atomic<uint8_t> radioStatus{static_cast<uint8_t>(RadioStatus::IDLE)};
static TaskHandle_t radioTaskHandle = nullptr;

static void setRadioStatus(RadioStatus s) { radioStatus = static_cast<uint8_t>(s); }
static RadioCmd peekRadioCmd() { return static_cast<RadioCmd>(radioCmd.load()); }
static RadioCmd takeRadioCmd() {
    return static_cast<RadioCmd>(radioCmd.exchange(static_cast<uint8_t>(RadioCmd::NONE)));
}

RadioStatus radioGetStatus() { return static_cast<RadioStatus>(radioStatus.load()); }

// Connects WiFi with a timeout. Returns early (false) if a STOP was requested.
static bool radioConnectWifi() {
    if (WiFi.status() == WL_CONNECTED) return true;
    setRadioStatus(RadioStatus::CONNECTING_WIFI);
    debugln("Radio: connecting WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI, PASSWORD);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (peekRadioCmd() == RadioCmd::STOP) return false;
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            debugln("Radio: WiFi timeout");
            WiFi.disconnect();
            setRadioStatus(RadioStatus::NO_WIFI);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    debugln("Radio: WiFi connected");
    return true;
}

// Waits up to ms, but returns immediately if a command arrives.
static void radioWait(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms && peekRadioCmd() == RadioCmd::NONE) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void radioTask(void*) {
    bool needConnect = true;

    player.begin(-1, false);        // set up decoder/output, don't open a station yet
    player.setAutoNext(false);      // on timeout don't jump stations; we retry the selected one
    urlSource.setTimeoutAutoNext(NO_DATA_TIMEOUT_MS);
    urlStream.setTimeout(URL_TIMEOUT_MS);

    for (;;) {
        RadioCmd cmd = takeRadioCmd();
        if (cmd == RadioCmd::STOP) break;
        if (cmd == RadioCmd::SELECT) needConnect = true;

        if (!radioConnectWifi()) {
            needConnect = true;
            radioWait(RETRY_DELAY_MS);
            continue;
        }

        if (needConnect) {
            needConnect = false;
            uint8_t station = currentStation;
            setRadioStatus(RadioStatus::CONNECTING);
            debug("Radio: opening ");
            debugln(urls[station]);
            player.setIndex(station); // blocks at most ~URL_TIMEOUT_MS
            if (peekRadioCmd() != RadioCmd::NONE) continue; // user already moved on
            if (!urlStream) {
                debugln("Radio: station failed");
                player.stop();
                setRadioStatus(RadioStatus::STATION_FAILED);
                needConnect = true;
                radioWait(RETRY_DELAY_MS);
                continue;
            }
            setRadioStatus(RadioStatus::PLAYING);
        }

        if (player.isActive()) {
            if (player.copy() == 0) vTaskDelay(1); // no data yet: don't hog the CPU
        } else {
            // copy() timed out without data (stream died) -> retry the same station
            debugln("Radio: stream lost");
            setRadioStatus(RadioStatus::STATION_FAILED);
            needConnect = true;
            radioWait(RETRY_DELAY_MS);
        }
    }

    debugln("Radio: stopping");
    player.end();
    urlStream.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    setRadioStatus(RadioStatus::IDLE);
    radioTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void radioStart() {
    if (radioTaskHandle != nullptr) return;
    radioCmd = static_cast<uint8_t>(RadioCmd::NONE);
    // Core 1 (WiFi runs on core 0), priority 2 so audio keeps flowing even if loop() is busy.
    xTaskCreatePinnedToCore(radioTask, "radio", 12288, nullptr, 2, &radioTaskHandle, 1);
}

static bool radioRunning() { return radioTaskHandle != nullptr; }

// Non-blocking: the task finishes whatever network call it is in and then shuts down.
static void radioStop() {
    if (radioRunning()) radioCmd = static_cast<uint8_t>(RadioCmd::STOP);
}

static void radioSelectStation(int8_t offset) {
    currentStation = (currentStation + offset + sizeOfUrls) % sizeOfUrls;
    currentRadioStationName = radioStationNames[currentStation];
    radioCmd = static_cast<uint8_t>(RadioCmd::SELECT);
}

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
    if (radioRunning()) {
        debugln(" radio stop");
        radioStop();
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

void copierInOutCopy() {
    copierInOut.copy();
}

void audioNext() {
    if (a2dp_sink.is_connected()) {
        a2dp_sink.next();
    } 
    else if (radioRunning()) {
        radioSelectStation(1);
    } 
}

void audioPrevious() {
    if (a2dp_sink.is_connected()) {
        a2dp_sink.previous();
    } 
    else if (radioRunning()) {
        radioSelectStation(-1);
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