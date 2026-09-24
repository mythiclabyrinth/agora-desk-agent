#pragma once

enum class Phase { Idle, Listening, Done, Failed };

class Feedback {
 public:
  void begin();
  void follow(Phase phase);

 private:
  void allOff();
  void attention();
  void success();
  void error();
  void beep(int duration);
  void blink();

  Phase _phase = Phase::Idle;
  unsigned long _lastBlinkAt = 0;
  bool _blinkOn = false;
};

extern Feedback feedback;
