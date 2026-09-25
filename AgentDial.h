#pragma once

#include <Arduino.h>

// Agents in the dial's fixed order; the position cue beeps index + 1 times.
constexpr int DIAL_AGENT_COUNT = 3;

// Where `steps` clicks from `current` land (indices into claude, cursor,
// codex), skipping agents that are not ready and wrapping both ways. A
// `current` that is not ready, or -1, is just a gap: the first click lands on
// the nearest ready agent in the turning direction. -1 when none is ready.
// Pure, so a host test can cover it.
inline int dialPick(const bool ready[DIAL_AGENT_COUNT], int current, int steps) {
  int count = 0;
  for (int i = 0; i < DIAL_AGENT_COUNT; i++) count += ready[i] ? 1 : 0;
  if (!count) return -1;
  if (!steps) return current;
  int dir = steps > 0 ? 1 : -1;
  int at = current;
  // Unknown: start just outside the ring so the first click lands on an end.
  if (at < 0 || at >= DIAL_AGENT_COUNT) at = dir > 0 ? DIAL_AGENT_COUNT - 1 : 0;
  for (int n = steps > 0 ? steps : -steps; n > 0; n--) {
    do {
      at = (at + dir + DIAL_AGENT_COUNT) % DIAL_AGENT_COUNT;
    } while (!ready[at]);
  }
  return at;
}

// Turns dial clicks into the hands-free agent. The choice applies in RAM at
// once; the "which agent" beeps wait until the knob rests, and the flash
// write waits longer. Neither happens while a recording is open.
class AgentDial {
 public:
  void begin();
  void update();
  // Cues played so far (turn at rest, or a press); a change means the user
  // was just told the hands-free agent.
  uint32_t cueCount() const { return _cues; }

 private:
  enum class Cue : uint8_t { None, Position, OnlyOne, NoneReady };

  void turn(int steps, unsigned long now);
  void playCue();

  Cue _cue = Cue::None;
  unsigned long _cueAt = 0;
  bool _spinChanged = false;  // this spin moved the selection (until its cue plays)
  bool _unsaved = false;
  unsigned long _saveAt = 0;
  unsigned long _pressedAt = 0;
  uint32_t _cues = 0;
};

extern AgentDial agentDial;
