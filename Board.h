#pragma once

#include <Arduino.h>

// ESP32-S3 header on this board, top to bottom beside the USB socket:
// 5V, GND, 13, 12, 11, 10, 9, 8, 46, 3, 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3.
// LED is GPIO 5, buzzer is GPIO 4 — the two pins just above RST.
// GPIO 3 and GPIO 46 are strapping pins, so they stay unused.
constexpr uint8_t LED_PIN = 5;
constexpr uint8_t BUZZER_PIN = 4;

// Push-to-talk button between GPIO 6 and GND (internal pull-up).
constexpr uint8_t TALK_BUTTON_PIN = 6;

// INMP441 microphone on I2S port 0. VDD 3V3, GND, L/R to GND (left slot).
constexpr int8_t MIC_BCLK_PIN = 12;   // SCK
constexpr int8_t MIC_WS_PIN = 11;     // WS
constexpr int8_t MIC_DATA_PIN = 13;   // SD

// MAX98357A amplifier on I2S port 1, 4 ohm speaker across the output.
// VIN 5V, GND, SD and GAIN left unconnected.
constexpr int8_t AMP_BCLK_PIN = 16;   // BCLK
constexpr int8_t AMP_LRC_PIN = 15;    // LRC
constexpr int8_t AMP_DATA_PIN = 17;   // DIN

// 16 kHz mono is what Whisper wants and keeps a 15 s clip under 500 KB.
constexpr uint32_t MIC_SAMPLE_RATE = 16000;
constexpr unsigned long RECORD_MAX_MS = 15000;
constexpr unsigned long RECORD_MIN_MS = 400;
// The INMP441 sits well below full scale for speech at desk distance.
constexpr int MIC_GAIN = 3;

constexpr char AP_SSID[] = "Esp32-Agent";
constexpr char AP_PASSWORD[] = "agent-setup";
constexpr char MDNS_HOST[] = "esp32-agent";

constexpr unsigned long LISTEN_TIMEOUT_MS = 180000;
constexpr unsigned long LISTEN_POLL_MS = 2000;
constexpr unsigned long WIFI_BOOT_WAIT_MS = 12000;
constexpr unsigned long WIFI_JOIN_WAIT_MS = 15000;

constexpr size_t MAX_USER_CHARS = 2000;
constexpr size_t MAX_REPLY_CHARS = 12000;
