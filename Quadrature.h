#pragma once

#include <stdint.h>

// Full-step quadrature decoder after Ben Buxton's state table. Every edge on
// either line moves the state along the Gray-code sequence; a count comes
// out only when a whole cycle completes and the lines are back at rest.
// Contact bounce is a flip back to the previous state, which the table
// simply follows, so it never produces a count and needs no delays. Invalid
// jumps (both lines changing at once) return to the start state.
//
// `pins` is (CLK << 1) | DT, both high (3) being the rest position between
// detents. Clockwise on a KY-040 is CLK falling first: 3 -> 1 -> 0 -> 2 -> 3.
class QuadratureDecoder {
 public:
  // +1 on a completed clockwise cycle, -1 counter-clockwise, 0 otherwise.
  // Safe to call from an ISR (in IRAM, table in DRAM).
  int8_t step(uint8_t pins);
  void reset() { _state = 0; }

 private:
  uint8_t _state = 0;
};
