#pragma once

#include <Arduino.h>

// ESP32-S3 header on this board, top to bottom beside the USB socket:
// GND, 5V, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3.
// GPIO 7 and 8 are free. 3 and 46 are strapping pins, so only parts that are
// safe at reset go there: 46 must not be pulled high while GPIO 0 is low (the
// upload path), and 3 is ignored at boot unless the JTAG-select eFuse is burned.
// Off this header, 35-37 belong to the octal PSRAM, 26-32 to flash, 19/20 to USB.
// Buttons (talk, mute, knob switch) go to GND and use the internal pull-up.
constexpr uint8_t LED_PIN = 5;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 40;
constexpr uint8_t BUZZER_PIN = 4;

constexpr uint8_t TALK_BUTTON_PIN = 6;
// Toggles mute: wake-word listening off (the talk button still works). A bare
// button to GND can only pull 46 low, which is its safe level at reset.
constexpr uint8_t MUTE_BUTTON_PIN = 46;
// Blue LED, lit while the mic listens (wake word armed, or recording). It
// drops ~3 V, so from a 3.3 V pin it needs 47-100 ohm, not 220-330.
constexpr uint8_t LISTEN_LED_PIN = 18;

// KY-040 rotary encoder: picks the hands-free agent. Power it from 3V3, not
// 5V: its 10k pull-ups go to "+", and the S3's GPIOs are not 5 V tolerant.
// The internal pull-ups are on too, as many boards leave SW's unpopulated.
constexpr uint8_t ENCODER_CLK_PIN = 9;
constexpr uint8_t ENCODER_DT_PIN = 10;
// SW sits on 3 because some modules pull it up (R3), which 46 cannot take.
constexpr uint8_t ENCODER_SW_PIN = 3;
// Quadrature cycles per click; 2 for a unit that needs two.
constexpr int DIAL_STEPS_PER_DETENT = 1;
// CLK/DT labels vary between makers; set if clockwise selects the previous agent.
constexpr bool DIAL_REVERSE = false;
// The "which agent" beeps wait for the knob to rest (a fast spin beeps once)...
constexpr unsigned long DIAL_BEEP_REST_MS = 350;
// ...and the flash write waits longer, so fidgeting does not wear NVS.
constexpr unsigned long DIAL_SAVE_REST_MS = 1500;
// One beep per place (claude 1, cursor 2, codex 3); loop() blocks meanwhile.
constexpr unsigned long DIAL_CUE_BEEP_MS = 60;
constexpr unsigned long DIAL_CUE_GAP_MS = 140;
// A shorter knob press replays the cue; longer holds are reserved.
constexpr unsigned long DIAL_SHORT_PRESS_MS = 800;

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

// One 10 ms block feeds both the wake-word frontend and the VAD.
constexpr uint32_t MIC_BLOCK_MS = 10;
// PSRAM ring that recordings copy from. Power of two so the sample counter
// wraps without a seam; 4 s covers a loop() stalled on a beep or web request.
constexpr size_t MIC_RING_SAMPLES = 65536;
constexpr size_t MIC_RING_SAMPLES_NO_PSRAM = 8192;  // 0.5 s in internal RAM
// Button/page recordings start this far back, keeping the first syllable.
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
// No echo cancellation, so the detector is deaf while the speaker or buzzer
// sounds and this long after; a spoken reply may contain the phrase.
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
