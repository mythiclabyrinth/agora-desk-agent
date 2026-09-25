#include "Dial.h"

#include "Board.h"

#include <hal/gpio_ll.h>
#include <soc/gpio_struct.h>

Dial dial;

void Dial::begin() {
  pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
  pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
  _button.begin(ENCODER_SW_PIN);
  _decoder.reset();
  // Both lines, both edges: the decoder needs every Gray-code step to tell a
  // real detent from bounce.
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), onEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN), onEdge, CHANGE);
}

// Runs on every edge, so it stays tiny: read both lines straight from the
// GPIO register (digitalRead is not guaranteed to be in IRAM), step the
// table, bump the counter. No Serial, no allocation.
void IRAM_ATTR Dial::onEdge() {
  uint8_t pins = (gpio_ll_get_level(&GPIO, ENCODER_CLK_PIN) ? 2 : 0) |
                 (gpio_ll_get_level(&GPIO, ENCODER_DT_PIN) ? 1 : 0);
  portENTER_CRITICAL_ISR(&dial._mux);
  dial._cycles += dial._decoder.step(pins);
  portEXIT_CRITICAL_ISR(&dial._mux);
}

int Dial::takeSteps() {
  portENTER_CRITICAL(&_mux);
  int cycles = _cycles;
  _cycles = 0;
  portEXIT_CRITICAL(&_mux);
  if (!cycles) return 0;
  cycles += _residue;
  int steps = cycles / DIAL_STEPS_PER_DETENT;
  _residue = cycles - steps * DIAL_STEPS_PER_DETENT;
  return DIAL_REVERSE ? -steps : steps;
}
