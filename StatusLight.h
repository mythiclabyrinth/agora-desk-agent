#pragma once

#include <Arduino.h>

#include "Feedback.h"
#include "VoiceFlow.h"

// The board's own WS2812 (GPIO 48, no wiring) is the desk's one light. At
// rest it is blue while the mic listens for the wake word and dark when
// muted or not ready. Through an exchange: amber while the mic is open (talk
// button, wake word, page mic), dimmer amber while the clip is transcribed, a
// short white flash as the message goes out, breathing violet while the desk
// waits for the reply, green when it lands, red when it fails, then back to
// blue or dark. The colour steps run from loop(); nothing here blocks.
class StatusLight {
 public:
  void begin();
  // `listening`: the wake word is armed right now (the policy, not the brief
  // deaf windows after a beep, so the light does not flicker).
  void follow(Phase chat, VoicePhase voice, bool listening);
  void update();

 private:
  void show(uint8_t r, uint8_t g, uint8_t b);
  void flash(uint8_t r, uint8_t g, uint8_t b, unsigned long ms);
  void resting();

  Phase _chat = Phase::Idle;
  VoicePhase _voice = VoicePhase::Idle;
  bool _listening = false;
  unsigned long _flashUntil = 0;
  unsigned long _breatheFrom = 0;
  uint8_t _lastLevel = 255;
};

extern StatusLight statusLight;
