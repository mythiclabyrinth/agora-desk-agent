// Desk client for the Claude, Cursor, and Codex bridges.
//
// Those bridges stay on the computer and stay dialed into Agora. This
// board hosts the page you talk from. It posts what you type into the
// Agora channel you configured, mentioning the agent you picked, then
// listens on that channel until the agent's reply lands.
//
// Wire an LED (with a resistor) from GPIO 5 to ground, and an active
// buzzer from GPIO 4 to ground. Those are the two pins just above RST
// on this ESP32-S3. Leave GPIO 3 and GPIO 46 alone; they are strapping pins.
//
// First boot opens a setup network: Esp32-Agent / agent-setup.
// Open http://192.168.4.1 — once Wi-Fi joins, also http://esp32-agent.local.

#include "ChatClient.h"
#include "Config.h"
#include "Feedback.h"
#include "Portal.h"
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
  webUi.begin();

  Serial.println("Ready");
}

void loop() {
  portal.update();
  chatClient.update();
  feedback.follow(chatClient.phase());
  webUi.handle();
}
