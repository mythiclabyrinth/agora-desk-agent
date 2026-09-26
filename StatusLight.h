#pragma once

#include <Arduino.h>

#include "Feedback.h"

// The board's own WS2812 (GPIO 48, no wiring) follows a chat exchange: a
// short white flash as the message goes out, a slow breathing blue while the
// desk waits for the reply, green when it lands, red when it fails, dark at
// rest. The colour steps run from loop(); nothing here blocks.
class StatusLight {
 public:
  void begin();
  void follow(Phase phase);
  void update();

 private:
  void show(uint8_t r, uint8_t g, uint8_t b);
  void flash(uint8_t r, uint8_t g, uint8_t b, unsigned long ms);

  Phase _phase = Phase::Idle;
  unsigned long _flashUntil = 0;
  unsigned long _breatheFrom = 0;
  uint8_t _lastLevel = 255;
};

extern StatusLight statusLight;
