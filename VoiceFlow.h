#pragma once

#include <Arduino.h>

#include "Config.h"

// Hold the button and talk; let go to send. The clip goes to Groq for text,
// the text goes through the same ChatClient the web page uses, and the reply
// is read out by Groq through the speaker.
enum class VoicePhase { Idle, Recording, Transcribing, Waiting, Speaking, Done, Failed };

// Who started this exchange: the physical button, the page's mic button, or
// a typed message whose reply should be read aloud.
enum class VoiceSource { Button, Page, Typed };

struct VoiceStatus {
  VoicePhase phase = VoicePhase::Idle;
  VoiceSource source = VoiceSource::Button;
  uint32_t job = 0;
  String agentKey;
  String heard;
  String reply;
  String error;
  unsigned long recordedMs = 0;
};

class VoiceFlow {
 public:
  void begin();
  void update();
  VoiceStatus status() const;
  // True while the mic is open or a clip is being sent; the LED stays lit.
  bool holdingLed() const;
  bool busy() const;

  // The page's mic button: open the mic for `agentKey`, then send on stop.
  bool startFromPage(const String &agentKey, String &error);
  bool stopFromPage(String &error);
  // A typed message is in flight on the ChatClient; read its reply aloud.
  bool speakWhenDone(uint32_t chatJob, const String &agentKey, String &error);

 private:
  void readButton();
  void onPress();
  void onRelease();
  bool beginRecording(const String &agentKey, VoiceSource source, String &error);
  void finishRecording();
  void sendRecording();
  void speakReply();
  // `chime` is false when the chat phase change already played the error beeps.
  void fail(const String &message, bool chime = true);

  VoicePhase _phase = VoicePhase::Idle;
  VoiceSource _source = VoiceSource::Button;
  uint32_t _job = 0;
  uint32_t _chatJob = 0;
  String _agentKey;
  String _heard;
  String _reply;
  String _error;
  unsigned long _recordedMs = 0;
  unsigned long _speakAt = 0;

  bool _pressed = false;
  bool _rawPressed = false;
  unsigned long _rawChangedAt = 0;
};

const char *voicePhaseName(VoicePhase phase);
const char *voiceSourceName(VoiceSource source);

extern VoiceFlow voiceFlow;
