// Desk client for the Claude, Cursor, and Codex bridges.
//
// Those bridges stay on the computer and stay dialed into Agora. This
// board hosts the page you talk from. It posts what you type into the
// Agora channel you configured, mentioning the agent you picked, then
// listens on that channel until the agent's reply lands.
//
// Hold the talk button and speak, and the same thing happens by voice:
// the INMP441 records, Groq (or OpenAI) turns it into text, the reply comes
// back through the same provider's speech and the MAX98357A speaker. Add the
// key under Settings › Voice.
//
// Hands-free: unless muted (the GPIO 7 mute button or Settings › Voice), say
// the phrase, wait for two beeps, speak,
// and pause. A microWakeWord model runs on the board (WakeModel.h, currently
// "Hey Jarvis"; tools/wake/README.md explains training "Hey Agora"), and an
// energy VAD ends the recording when you stop talking.
//
// The chat page can do the same without the desk hardware: the browser
// records with its own microphone, the board forwards the clip for
// transcription and fetches the spoken reply as a WAV for the page to play
// (/api/voice/transcribe, /api/voice/say). Keys never leave the board.
// Browsers only share the mic with secure pages, so over plain HTTP that
// needs Chrome's "insecure origin as secure" flag for the board's address.
//
// Wiring (ESP32-S3, right-hand header):
//   LED (with resistor)  GPIO 5 -> LED -> GND
//   Active buzzer        GPIO 4 -> buzzer -> GND
//   Talk button          GPIO 6 -> button -> GND (internal pull-up)
//   Mute button          GPIO 7 -> button -> GND (internal pull-up)
//   Blue listening LED   GPIO 8 -> 47-100 ohm -> LED -> GND (blue drops ~3 V, so
//                        a bigger resistor leaves it very dim at 3.3 V). Lit
//                        while the mic listens: wake word armed, or recording.
// Still free on the usable header: GPIO 9, 10, 18.
//   INMP441 mic          SCK GPIO 12, WS GPIO 11, SD GPIO 13, L/R -> GND, VDD 3V3
//   MAX98357A amp        BCLK GPIO 16, LRC GPIO 15, DIN GPIO 17, VIN 5V, speaker on +/-
// Leave GPIO 0, 3, 45 and 46 alone (strapping pins), and 35-37 (octal PSRAM).
//
// Board settings: ESP32S3 Dev Module, PSRAM "OPI PSRAM" (the N16R8 has
// 8 MB; recordings live there), Flash Size 16MB, Partition Scheme
// "16M Flash (3MB APP/9.9MB FATFS)" — the default 1.3 MB app slot is nearly full.
//
// The web page lives in web/ (index.html, styles.css, js/*.js). Page.h is
// generated from it, gzipped: run `python3 web/build.py` after editing web/
// and before uploading. `python3 tools/preview.py` serves web/ locally.
//
// First boot opens a setup network: Esp32-Agent / agent-setup.
// Open http://192.168.4.1 — once Wi-Fi joins, also http://esp32-agent.local.

#include "ChatClient.h"
#include "Config.h"
#include "Feedback.h"
#include "Mic.h"
#include "Portal.h"
#include "VoiceFlow.h"
#include "Wake.h"
#include "WebUi.h"

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("========================");
  Serial.println(" ESP32 AGENT DESK");
  Serial.println("========================");

  feedback.begin();
  configStore.begin();
  portal.begin();
  // The wake engine first: the mic task starts feeding it the moment it runs.
  wakeWord.begin();
  mic.begin();
  voiceFlow.begin();
  webUi.begin();

  Serial.println("Ready");
}

void loop() {
  portal.update();
  chatClient.update();
  voiceFlow.update();
  feedback.follow(chatClient.phase(), voiceFlow.holdingLed());
  webUi.handle();
}
