#pragma once

#include <Arduino.h>

// ESP32-S3 header on this board, top to bottom beside the USB socket:
// 5V, GND, 13, 12, 11, 10, 9, 8, 46, 3, 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3.
// LED is GPIO 5, buzzer is GPIO 4 — the two pins just above RST.
// GPIO 3 and GPIO 46 are strapping pins, so they stay unused.
constexpr uint8_t LED_PIN = 5;
constexpr uint8_t BUZZER_PIN = 4;

constexpr char AP_SSID[] = "Esp32-Agent";
constexpr char AP_PASSWORD[] = "agent-setup";
constexpr char MDNS_HOST[] = "esp32-agent";

constexpr unsigned long LISTEN_TIMEOUT_MS = 180000;
constexpr unsigned long LISTEN_POLL_MS = 2000;
constexpr unsigned long WIFI_BOOT_WAIT_MS = 12000;
constexpr unsigned long WIFI_JOIN_WAIT_MS = 15000;

constexpr size_t MAX_USER_CHARS = 2000;
constexpr size_t MAX_REPLY_CHARS = 12000;
