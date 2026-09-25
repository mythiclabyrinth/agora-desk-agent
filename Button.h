#pragma once

#include <Arduino.h>

// A push button to GND with the internal pull-up, debounced by time. Shared
// by the talk button (VoiceFlow) and the dial's knob switch (Dial).
class DebouncedButton {
 public:
  void begin(uint8_t pin);
  // +1 on a settled press, -1 on a settled release, 0 otherwise.
  int8_t poll();
  // How long the button has been down, from the press's first edge; 0 while released.
  unsigned long heldMs() const { return _pressed ? millis() - _pressedAt : 0; }

 private:
  uint8_t _pin = 0;
  bool _pressed = false;
  bool _raw = false;
  unsigned long _changedAt = 0;
  unsigned long _pressedAt = 0;
};
