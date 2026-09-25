#pragma once

#include <Arduino.h>

// ESP32-S3 header on this board, top to bottom beside the USB socket:
// GND, 5V, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3.
// LED is GPIO 5, buzzer is GPIO 4 — the two pins just above RST.
// GPIO 3 and GPIO 46 are strapping pins, so they stay unused.
constexpr uint8_t LED_PIN = 5;
constexpr uint8_t BUZZER_PIN = 4;

// Push-to-talk button between GPIO 6 and GND (internal pull-up).
constexpr uint8_t TALK_BUTTON_PIN = 6;

// Mute button between GPIO 7 and GND (internal pull-up). Each press toggles
// mute; muted means wake-word listening is off (the talk button still works).
constexpr uint8_t MUTE_BUTTON_PIN = 7;
// Blue "listening" LED: GPIO 8 -> resistor -> LED -> GND. Lit whenever the
// board's mic is listening: wake word armed (not muted), or any recording.
// A blue LED drops about 2.8-3.2 V, which leaves only a few hundred mV
// across the resistor from a 3.3 V pin: use roughly 47-100 ohm, not the usual
// 220-330, or it will barely glow.
constexpr uint8_t LISTEN_LED_PIN = 8;
// On this breadboard's usable header (GND, 5V, 13, 12, 11, 10, 9, 46, 3, 8,
// 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3) the free pins left are 9, 10 and 18;
// 3 and 46 are strapping pins. Elsewhere, 35-37 belong to the octal PSRAM,
// 26-32 to flash, 19/20 to USB.

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

// The mic task reads 10 ms blocks: the wake-word frontend steps 10 ms and the
// VAD judges 10 ms frames, so one block feeds both.
constexpr uint32_t MIC_BLOCK_MS = 10;
// Every sample lands in a PSRAM ring first; recordings copy out of it. Power of
// two so the running sample counter can wrap without a seam. 65536 samples is
// 4 s, which covers a loop() that stalls on a beep or a slow web request.
constexpr size_t MIC_RING_SAMPLES = 65536;
// Without PSRAM the ring shrinks to half a second in internal RAM.
constexpr size_t MIC_RING_SAMPLES_NO_PSRAM = 8192;
// A button or page recording starts this far in the past, so the first
// syllable spoken as the button goes down is not lost.
constexpr unsigned long MIC_PREROLL_MS = 300;
// Core 0 beside Wi-Fi (priority 23) and lwIP (18), well below both.
constexpr uint8_t MIC_TASK_CORE = 0;
constexpr uint8_t MIC_TASK_PRIORITY = 5;
constexpr uint32_t MIC_TASK_STACK = 8192;

// Wake word. The model's own cutoff (WakeModel.h) is "medium"; low and high
// move it by this much, clamped to the range below.
constexpr float WAKE_CUTOFF_SHIFT = 0.04f;
constexpr float WAKE_CUTOFF_MIN = 0.50f;
constexpr float WAKE_CUTOFF_MAX = 0.99f;
// After a detection the detector ignores the phrase's own tail.
constexpr unsigned long WAKE_REFRACTORY_MS = 2000;
// No echo cancellation (one INMP441, no reference feed from the amp), so the
// detector is deaf while the speaker or buzzer sounds and for this long after;
// a spoken reply may well contain the wake phrase.
constexpr unsigned long WAKE_QUIET_AFTER_SOUND_MS = 700;
// Streaming models need about a second of features to settle after re-arming.
constexpr unsigned long WAKE_WARMUP_MS = 1000;
// Tensor arena ceiling. The manifest asks for ~23 KB; probing may grow it.
constexpr size_t WAKE_ARENA_MAX = 64 * 1024;

// Energy VAD on 10 ms frames, levels in dB re 1 LSB (full scale is ~90 dB).
// Speech is the level this far above the adaptive noise floor...
constexpr float VAD_MARGIN_DB = 10.0f;
// ...for this many frames in a row, and stays "speech" through short gaps.
constexpr uint8_t VAD_ONSET_FRAMES = 3;
constexpr unsigned long VAD_HANGOVER_MS = 200;
// The floor falls fast (a door closing) and rises slowly (someone talking).
constexpr float VAD_FLOOR_FALL = 0.2f;              // fraction of the gap per frame
constexpr float VAD_FLOOR_RISE_DB_PER_S = 1.5f;
// A digitally silent mic would put the floor at 0 dB and make a breath speech.
constexpr float VAD_FLOOR_MIN_DB = 30.0f;
// Wake recordings end on their own: after this much quiet following speech,
constexpr unsigned long VAD_END_SILENCE_MS = 900;
// or cancelled if no speech starts within this long of the wake chime,
constexpr unsigned long VAD_NO_SPEECH_MS = 4000;
// where "speech" means at least this much of it. RECORD_MAX_MS still caps.
constexpr unsigned long VAD_MIN_SPEECH_MS = 200;
// The chime itself is loud enough to count as speech; start judging after it
// ends plus more than the hangover, or its tail would pass for a word.
constexpr unsigned long VAD_CHIME_SKIP_MS = 300;

constexpr char AP_SSID[] = "Esp32-Agent";
constexpr char AP_PASSWORD[] = "agent-setup";
constexpr char MDNS_HOST[] = "esp32-agent";

constexpr unsigned long LISTEN_TIMEOUT_MS = 180000;
constexpr unsigned long LISTEN_POLL_MS = 2000;
constexpr unsigned long WIFI_BOOT_WAIT_MS = 12000;
constexpr unsigned long WIFI_JOIN_WAIT_MS = 15000;

constexpr size_t MAX_USER_CHARS = 2000;
constexpr size_t MAX_REPLY_CHARS = 12000;
