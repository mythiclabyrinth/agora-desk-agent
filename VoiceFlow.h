#pragma once

#include <Arduino.h>

#include "Button.h"
#include "Config.h"
#include "Mic.h"

// Hold the button and talk; let go to send. Or say the wake phrase, talk,
// and pause. The clip is transcribed, sent through the same ChatClient the
// web page uses, and the reply is spoken through the desk speaker.
enum class VoicePhase { Idle, Recording, Transcribing, Waiting, Speaking, Done, Failed };

// Who started this exchange: the physical button, the page's mic button, a
// typed message whose reply should be read aloud, or the wake word.
enum class VoiceSource { Button, Page, Typed, Wake };

struct VoiceStatus {
  VoicePhase phase = VoicePhase::Idle;
  VoiceSource source = VoiceSource::Button;
  uint32_t job = 0;
  String agentKey;
  String heard;
  String reply;
  String error;
  unsigned long recordedMs = 0;
  // Failed because the clip held nothing usable (too short, empty, only the
  // wake phrase): "say it again" rather than a fault.
  bool missed = false;
};

class VoiceFlow {
 public:
  void begin();
  void update();
  VoiceStatus status() const;
  // Cheap phase check for code that runs every loop pass (status() copies strings).
  VoicePhase phase() const { return _phase; }
  // True while the mic is open or a clip is being sent; the LED stays lit.
  bool holdingLed() const;
  bool busy() const;
  // Called on every phase change. Transcribing and Speaking block loop() as
  // soon as they begin, so polling alone would never see them.
  void onPhaseChange(void (*fn)()) { _onPhase = fn; }

  // The page's mic button: open the mic for `agentKey`, then send on stop.
  bool startFromPage(const String &agentKey, String &error);
  bool stopFromPage(String &error);
  // A typed message is in flight on the ChatClient; read its reply aloud.
  bool speakWhenDone(uint32_t chatJob, const String &agentKey, String &error);

  // Wake-word listening is switched on (not muted) and the engine loaded.
  bool wakeListening() const;
  // The detector is armed right now (policy, not the brief deaf windows).
  bool wakeArmed() const { return _wakeArmed; }
  // Unmute / mute wake listening and save it; the page and the mute button
  // both come through here.
  bool setWakeEnabled(bool on, String &error);
  void setWakeSensitivity(const String &level);

 private:
  void readButtons();
  void onPress();
  void onRelease();
  void onMuteButton();
  void updateWake();
  void onWake(uint32_t samplePos);
  bool wakeReady();
  bool wakeHeardSpeech(const VadStats &now) const;
  void checkWakeEnd();
  void cancelWake(const char *why);
  bool beginRecording(const String &agentKey, VoiceSource source, String &error, unsigned long preRollMs);
  void finishRecording();
  void sendRecording();
  void speakReply();
  void setPhase(VoicePhase phase);
  // `chime` is false when the chat phase change already played the error beeps.
  void fail(const String &message, bool chime = true, bool missed = false);

  VoicePhase _phase = VoicePhase::Idle;
  void (*_onPhase)() = nullptr;
  bool _missed = false;
  VoiceSource _source = VoiceSource::Button;
  uint32_t _job = 0;
  uint32_t _chatJob = 0;
  String _agentKey;
  String _heard;
  String _reply;
  String _error;
  unsigned long _recordedMs = 0;
  unsigned long _speakAt = 0;

  DebouncedButton _talk;
  DebouncedButton _muteButton;
  bool _wakeArmed = false;

  // Wake recording endpointing: VAD counters from when judging began.
  uint32_t _vadFromFrame = 0;  // judge frames from here (after the chime)
  bool _vadStarted = false;
  VadStats _vadFrom;
  // The slower readiness checks (keys, agent, Wi-Fi), refreshed once a second.
  bool _wakeReady = false;
  unsigned long _wakeCheckedAt = 0;
  unsigned long _wakeTracedAt = 0;
};

const char *voicePhaseName(VoicePhase phase);
const char *voiceSourceName(VoiceSource source);

extern VoiceFlow voiceFlow;
