#pragma once

#include <Arduino.h>

#include "Feedback.h"  // Phase
#include "VoiceFlow.h"

struct ScreenLines;

// Decides what the LCD shows. It polls the other modules (none of them know
// the display exists), turns their state into the main screen (Screens.h),
// and raises a notice on each transition worth telling: reply, error, mute,
// dial, wake, Wi-Fi.
class StatusScreen {
 public:
  // Before the slow parts of setup(), so the boot screen shows during Wi-Fi join.
  void begin();
  void update();

 private:
  // Each returns true when something changed.
  bool checkWifi();
  bool checkChat(unsigned long now);
  bool checkVoice();
  bool checkControls();
  void notify(const ScreenLines &lines, unsigned long ms);
  void showMain(unsigned long now);

  bool _started = false;
  bool _wifi = false;
  String _address;
  VoicePhase _voicePhase = VoicePhase::Idle;
  Phase _chatPhase = Phase::Idle;
  const char *_chatAgent = "";
  unsigned long _waitSince = 0;
  bool _wakeOn = false;
  String _agent;
  uint32_t _cues = 0;
  unsigned long _shownAt = 0;
};

extern StatusScreen statusScreen;
