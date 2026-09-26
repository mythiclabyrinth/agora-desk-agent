#pragma once

#include <Arduino.h>

#include "Feedback.h"
#include "VoiceFlow.h"

// The board's own WS2812 (GPIO 48, no wiring) follows an exchange: amber
// while the mic is open (talk button, wake word, page mic) and dimmer while
// the clip is transcribed, a short white flash as the message goes out,
// breathing violet while the desk waits for the reply, green when it lands,
// red when it fails, dark at rest. Blue is left to the listening LED. The
// colour steps run from loop(); nothing here blocks.
class StatusLight {
 public:
  void begin();
  void follow(Phase chat, VoicePhase voice);
  void update();

 private:
  void show(uint8_t r, uint8_t g, uint8_t b);
  void flash(uint8_t r, uint8_t g, uint8_t b, unsigned long ms);
  void resting();

  Phase _chat = Phase::Idle;
  VoicePhase _voice = VoicePhase::Idle;
  unsigned long _flashUntil = 0;
  unsigned long _breatheFrom = 0;
  uint8_t _lastLevel = 255;
};

extern StatusLight statusLight;
