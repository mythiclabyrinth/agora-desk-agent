// Desk client for the Claude, Cursor, and Codex bridges.
//
// Those bridges stay on the computer and stay dialed into Agora. This
// board hosts the page you talk from. It posts what you type into the
// Agora channel you configured, mentioning the agent you picked, then
// listens on that channel until the agent's reply lands.
//
// Hold the talk button and speak, and the same thing happens by voice: the
// INMP441 records, Groq or OpenAI transcribes it, and the reply is spoken
// through the MAX98357A. The buzzer marks each step; the blue LED is lit
// while the mic listens.
//
// Hands-free: unless muted, say the wake phrase, wait for two beeps, speak,
// and pause. A microWakeWord model runs on the board (WakeModel.h) and an
// energy VAD ends the recording; click the talk button once to send it now,
// twice to cancel it (the same goes for a recording started from the page).
//
// The talk button and the wake word reach the hands-free agent. The rotary
// dial sets it: turn to cycle through the configured agents (it beeps the
// place: claude 1, cursor 2, codex 3), press briefly to hear it again, hold
// to mute or unmute the wake word (Settings › Voice does the same).
//
// A 16x2 I2C LCD shows the hands-free agent and whether the desk is
// listening, each step of an exchange, and a notice for every event.
//
// The chat page can also use the browser's own mic and speaker; the board
// proxies the speech APIs (/api/voice/transcribe, /api/voice/say), so keys
// never leave it.
//
// Wiring (ESP32-S3; details and rationale in Board.h):
//   Active buzzer        GPIO 4 -> buzzer -> GND
//   Talk button          GPIO 6 -> button -> GND (internal pull-up)
//   Blue listening LED   GPIO 18 -> 47-100 ohm -> LED -> GND
//   KY-040 dial          CLK GPIO 9, DT GPIO 10, SW GPIO 3, + -> 3V3 (not 5V), GND
//   INMP441 mic          SCK GPIO 12, WS GPIO 11, SD GPIO 13, L/R -> GND, VDD 3V3
//   MAX98357A amp        BCLK GPIO 16, LRC GPIO 15, DIN GPIO 17, VIN 5V, speaker on +/-
//   16x2 LCD (PCF8574)   SDA GPIO 8, SCL GPIO 7, VCC 5V, GND (pull-ups to 3V3 only; see Board.h)
// Free: GPIO 5 and GPIO 46. 46 is a strapping pin: a bare button to GND is
// safe there, a part with a pull-up is not (see Board.h).
//
// Board settings: ESP32S3 Dev Module, PSRAM "OPI PSRAM" (recordings live
// there), Flash Size 16MB, Partition Scheme "16M Flash (3MB APP/9.9MB FATFS)"
// (the sketch does not fit the default 1.3 MB app slot).
//
// Page.h is generated from web/: run `python3 web/build.py` after editing it.
//
// First boot opens a setup network: Esp32-Agent / agent-setup.
// Open http://192.168.4.1 — once Wi-Fi joins, also http://esp32-agent.local.

#include "AgentDial.h"
#include "ChatClient.h"
#include "Config.h"
#include "Display.h"
#include "Feedback.h"
#include "Mic.h"
#include "Portal.h"
#include "StatusScreen.h"
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
  // Before the Wi-Fi join, which can take seconds, so the boot screen shows.
  display.begin();
  statusScreen.begin();
  portal.begin();
  // The wake engine first: the mic task starts feeding it the moment it runs.
  wakeWord.begin();
  mic.begin();
  voiceFlow.begin();
  agentDial.begin();
  webUi.begin();

  Serial.println("Ready");
}

void loop() {
  portal.update();
  chatClient.update();
  voiceFlow.update();
  // After voiceFlow, so a recording that just ended frees any held beep.
  agentDial.update();
  feedback.follow(chatClient.phase());
  statusScreen.update();
  webUi.handle();
}
