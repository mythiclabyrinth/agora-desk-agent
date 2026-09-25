#pragma once

#include <stdint.h>

enum class Phase { Idle, Listening, Done, Failed };

class Feedback {
 public:
  void begin();
  // Beeps on the chat's phase changes: sent, reply landed, failed.
  void follow(Phase phase);
  // A quick tick for the button: mic opened / mic closed.
  void tick();
  void error();
  // Wake word heard, mic open: two short beeps, unlike the button's tick.
  void wakeChime();
  // A hands-free recording gave up (nobody spoke): one long, low-key beep.
  void cancel();
  // Mute: one short beep when muting (or dropping a recording), two when
  // unmuting.
  void muteToggled(bool muted);
  // The dial's "which agent" cue: `position` evenly spaced short beeps.
  void agentCue(uint8_t position);
  // The blue LED: lit whenever the board's mic is listening.
  void listenLed(bool on);

 private:
  void attention();
  void success();
  void beep(int duration);

  Phase _phase = Phase::Idle;
  int8_t _listenLed = -1;  // unknown until the first listenLed() call
};

extern Feedback feedback;
