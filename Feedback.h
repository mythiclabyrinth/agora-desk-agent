#pragma once

#include <stdint.h>

enum class Phase { Idle, Listening, Done, Failed };

class Feedback {
 public:
  void begin();
  // `hold` keeps the LED lit regardless of phase: the mic is open or the
  // clip is being transcribed, so "the board is busy with you" not "waiting".
  void follow(Phase phase, bool hold = false);
  // A quick tick for the button: mic opened / mic closed.
  void tick();
  void error();
  // Wake word heard, mic open: two short beeps, unlike the button's tick.
  void wakeChime();
  // A hands-free recording gave up (nobody spoke): one long, low-key beep.
  void cancel();
  // Mute button: one short beep when muting (or dropping a recording), two
  // when unmuting.
  void muteToggled(bool muted);
  // The dial's "which agent": `position` evenly spaced short beeps (claude 1,
  // cursor 2, codex 3), so the choice can be told by ear.
  void agentCue(uint8_t position);
  // The blue LED: lit whenever the board's mic is listening.
  void listenLed(bool on);

 private:
  void allOff();
  void attention();
  void success();
  void beep(int duration);
  void blink();

  Phase _phase = Phase::Idle;
  unsigned long _lastBlinkAt = 0;
  bool _blinkOn = false;
  bool _held = false;
  int8_t _listenLed = -1;  // unknown until the first listenLed() call
};

extern Feedback feedback;
