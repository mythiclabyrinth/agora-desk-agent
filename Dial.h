#pragma once

#include <Arduino.h>

#include "Button.h"
#include "Quadrature.h"

// The KY-040 rotary encoder: hardware only, no idea what it selects.
// CHANGE interrupts on CLK and DT feed the quadrature decoder, which adds
// whole detents to a counter under a spinlock; loop() drains the counter
// with takeSteps(). The knob's switch is an ordinary debounced button.
// Nothing is wired? The pull-ups hold every line idle and the dial is silent.
class Dial {
 public:
  void begin();
  // Signed detents since the last call (clockwise positive), zeroing them.
  int takeSteps();
  // The knob's switch: +1 on a settled press, -1 on a settled release.
  int8_t pollPress() { return _button.poll(); }

 private:
  static void onEdge();

  QuadratureDecoder _decoder;  // ISR only
  int _cycles = 0;             // full quadrature cycles, only touched under _mux
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
  int _residue = 0;  // cycles short of a whole detent (DIAL_STEPS_PER_DETENT > 1)
  DebouncedButton _button;
};

extern Dial dial;
