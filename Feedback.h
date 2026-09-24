#pragma once

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
};

extern Feedback feedback;
