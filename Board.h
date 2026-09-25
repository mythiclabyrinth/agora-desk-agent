#pragma once

#include <Arduino.h>

// ESP32-S3 header on this board, top to bottom beside the USB socket:
// GND, 5V, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3.
//   13, 12, 11   INMP441 mic (SD, SCK, WS)
//   10, 9, 3     KY-040 dial (DT, CLK, knob switch)
//   8, 7         LCD I2C (SDA, SCL)
//   18           blue listening LED
//   17, 16, 15   MAX98357A amp (DIN, BCLK, LRC)
//   6            talk button
//   4            buzzer
// Free: GPIO 5, GPIO 46.
// 3 and 46 are strapping pins. 3 is ignored at boot unless the JTAG-select
// eFuse is burned, which a stock S3 does not have. 46 must not be pulled high
// while GPIO 0 is low (the upload path): a bare button to GND is safe there, a
// part with a pull-up is not.
// Off this header, 35-37 belong to the octal PSRAM, 26-32 to flash, 19/20 to USB.
// Buttons (talk, knob switch) go to GND and use the internal pull-up.
constexpr unsigned long BUTTON_DEBOUNCE_MS = 40;
constexpr uint8_t BUZZER_PIN = 4;

constexpr uint8_t TALK_BUTTON_PIN = 6;
// During a hands-free or page recording, one click sends it and two cancel
// it; a click is single once this long passes without a second press.
constexpr unsigned long TALK_DOUBLE_CLICK_MS = 350;
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
// Holding the knob this long toggles mute. The beep sounds while it is still
// held, so the user knows to let go; a shorter press replays the cue.
constexpr unsigned long DIAL_LONG_PRESS_MS = 800;

// INMP441 microphone on I2S port 0. VDD 3V3, GND, L/R to GND (left slot).
constexpr int8_t MIC_BCLK_PIN = 12;   // SCK
constexpr int8_t MIC_WS_PIN = 11;     // WS
constexpr int8_t MIC_DATA_PIN = 13;   // SD

// MAX98357A amplifier on I2S port 1, 4 ohm speaker across the output.
// VIN 5V, GND, SD unconnected. GAIN unconnected is 9 dB; GAIN to GND gives
// 12 dB and GAIN to VIN 15 dB, the cleanest way to more volume.
constexpr int8_t AMP_BCLK_PIN = 16;   // BCLK
constexpr int8_t AMP_LRC_PIN = 15;    // LRC
constexpr int8_t AMP_DATA_PIN = 17;   // DIN
// Speech from the APIs sits well under full scale; samples are multiplied
// by this (with clipping) on the way to the amp. 1 leaves them as sent.
constexpr int SPEAKER_GAIN = 2;
// The I2S DMA holds 60 ms and the speech APIs deliver at about real time
// with pauses, so this much of each piece waits in PSRAM before the amp
// starts; playback then runs that far behind the network.
constexpr unsigned long SPEAKER_PREBUFFER_MS = 600;

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

// Wake word. It fires when the mean of the model's last few probabilities
// passes a cutoff: the model's own (WakeModel.h) until the page's slider
// sets one, always within this range.
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
// The model's per-step output peaks for a few tens of ms; the page polls once
// a second, so the peak is held this long for the meter and the serial trace.
constexpr unsigned long WAKE_PEAK_HOLD_MS = 3000;
// Serial trace cadence while armed; it prints only when the peak is above this.
constexpr unsigned long WAKE_TRACE_MS = 1000;
constexpr float WAKE_TRACE_MIN = 0.05f;

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

// 16x2 HD44780 LCD on a PCF8574 I2C backpack. The backpack wants 5V for
// contrast, which puts its SDA/SCL pull-ups at 5V, and the S3's GPIOs are not
// 5 V tolerant: remove those pull-ups and fit 4.7k to 3V3, or use a BSS138
// level shifter.
constexpr uint8_t LCD_SDA_PIN = 8;
constexpr uint8_t LCD_SCL_PIN = 7;
constexpr uint32_t LCD_I2C_HZ = 100000;  // the PCF8574's rated maximum
// Most backpacks answer at 0x27, PCF8574A ones at 0x3F; boot probes both.
constexpr uint8_t LCD_I2C_ADDR = 0x27;
constexpr uint8_t LCD_I2C_ADDR_ALT = 0x3F;
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;
// Each character costs six expander writes (~1.5 ms at 100 kHz), so a task
// redraws only changed cells, at most this often.
constexpr unsigned long DISPLAY_REFRESH_MS = 100;
constexpr unsigned long DISPLAY_SPINNER_MS = 250;
// Lines wider than the screen scroll, after a pause on their start.
constexpr unsigned long DISPLAY_SCROLL_STEP_MS = 350;
constexpr unsigned long DISPLAY_SCROLL_HOLD_MS = 1000;
constexpr size_t DISPLAY_TEXT_MAX = 160;  // a scrolling error is cut here
// The backlight goes off after this long on an unchanged resting screen.
constexpr unsigned long DISPLAY_DIM_MS = 60000;
// Beside loop() at its priority, so the screen keeps moving while loop() blocks.
constexpr uint8_t DISPLAY_TASK_CORE = 1;
constexpr uint8_t DISPLAY_TASK_PRIORITY = 1;
constexpr uint32_t DISPLAY_TASK_STACK = 4096;
// How long each notice stays over the main screen.
constexpr unsigned long NOTICE_WAKE_MS = 1500;
constexpr unsigned long NOTICE_AGENT_MS = 2000;
constexpr unsigned long NOTICE_MUTE_MS = 2500;
constexpr unsigned long NOTICE_MISSED_MS = 3000;
constexpr unsigned long NOTICE_REPLY_MS = 4000;
constexpr unsigned long NOTICE_WIFI_MS = 4000;
constexpr unsigned long NOTICE_ERROR_MS = 5000;
constexpr unsigned long NOTICE_SETUP_MS = 5000;

constexpr char AP_SSID[] = "Esp32-Agent";
constexpr char AP_PASSWORD[] = "agent-setup";
constexpr char MDNS_HOST[] = "esp32-agent";

constexpr unsigned long LISTEN_TIMEOUT_MS = 180000;
constexpr unsigned long LISTEN_POLL_MS = 2000;
constexpr unsigned long WIFI_BOOT_WAIT_MS = 12000;
constexpr unsigned long WIFI_JOIN_WAIT_MS = 15000;

constexpr size_t MAX_USER_CHARS = 2000;
constexpr size_t MAX_REPLY_CHARS = 12000;
