#include "VoiceFlow.h"

#include "Audio.h"
#include "Board.h"
#include "ChatClient.h"
#include "Feedback.h"
#include "Portal.h"
#include "Speech.h"

namespace {

constexpr unsigned long DEBOUNCE_MS = 40;
constexpr unsigned long SPEAK_GRACE_MS = 1500;

}  // namespace

VoiceFlow voiceFlow;

const char *voicePhaseName(VoicePhase phase) {
  switch (phase) {
    case VoicePhase::Recording: return "recording";
    case VoicePhase::Transcribing: return "transcribing";
    case VoicePhase::Waiting: return "waiting";
    case VoicePhase::Speaking: return "speaking";
    case VoicePhase::Done: return "done";
    case VoicePhase::Failed: return "failed";
    default: return "idle";
  }
}

const char *voiceSourceName(VoiceSource source) {
  switch (source) {
    case VoiceSource::Page: return "page";
    case VoiceSource::Typed: return "typed";
    default: return "button";
  }
}

void VoiceFlow::begin() {
  pinMode(TALK_BUTTON_PIN, INPUT_PULLUP);
  _rawPressed = digitalRead(TALK_BUTTON_PIN) == LOW;
  _pressed = _rawPressed;
  _rawChangedAt = millis();
  audio.begin();
}

bool VoiceFlow::holdingLed() const {
  return _phase == VoicePhase::Recording || _phase == VoicePhase::Transcribing;
}

bool VoiceFlow::busy() const {
  return _phase == VoicePhase::Recording || _phase == VoicePhase::Transcribing ||
         _phase == VoicePhase::Waiting || _phase == VoicePhase::Speaking;
}

VoiceStatus VoiceFlow::status() const {
  VoiceStatus s;
  s.phase = _phase;
  s.source = _source;
  s.job = _job;
  s.agentKey = _agentKey;
  s.heard = _heard;
  s.reply = _reply;
  s.error = _error;
  s.recordedMs = _phase == VoicePhase::Recording ? audio.recordedMs() : _recordedMs;
  return s;
}

void VoiceFlow::readButton() {
  bool raw = digitalRead(TALK_BUTTON_PIN) == LOW;
  unsigned long now = millis();
  if (raw != _rawPressed) {
    _rawPressed = raw;
    _rawChangedAt = now;
    return;
  }
  if (raw == _pressed || now - _rawChangedAt < DEBOUNCE_MS) return;
  _pressed = raw;
  if (_pressed) onPress();
  else onRelease();
}

void VoiceFlow::update() {
  readButton();

  if (_phase == VoicePhase::Recording) {
    audio.pumpRecording();
    if (audio.recordingFull() || audio.recordedMs() >= RECORD_MAX_MS) {
      Serial.println("Voice: clip is at the limit, sending");
      finishRecording();
    }
    return;
  }

  if (_phase == VoicePhase::Waiting) {
    ListenStatus chat = chatClient.status();
    if (chat.job != _chatJob) {
      fail("A newer message replaced this one.", false);
      return;
    }
    if (chat.phase == Phase::Done) {
      // Speaking blocks the web server, so give the page a moment to fetch
      // the finished reply before the speaker takes over.
      if (!_speakAt) _speakAt = millis() + SPEAK_GRACE_MS;
      if (millis() < _speakAt) return;
      _speakAt = 0;
      _reply = chat.reply;
      speakReply();
    } else if (chat.phase == Phase::Failed) {
      fail(chat.error.length() ? chat.error : String("The agent did not reply."), false);
    }
  }
}

bool VoiceFlow::beginRecording(const String &agentKey, VoiceSource source, String &error) {
  if (busy()) {
    error = "The desk is already in a voice exchange.";
    return false;
  }
  // The web chat owns the ChatClient while it is listening; wait our turn.
  if (chatClient.phase() == Phase::Listening) {
    error = "The desk is still waiting on a typed message.";
    return false;
  }
  VoiceSettings voice = configStore.voice();
  if (!voice.sttReady()) {
    error = "Add the speech-to-text provider's API key under Settings › Voice.";
    return false;
  }
  if (!portal.staConnected()) {
    error = "The board is not on Wi-Fi yet.";
    return false;
  }
  if (!audio.micReady()) {
    error = "Microphone did not start. Check the INMP441 wiring.";
    return false;
  }
  AgentKind kind = ConfigStore::parseAgent(agentKey);
  if (kind == AgentKind::Unknown || !configStore.agent(kind).ready) {
    error = "Set up that agent under Settings › Agents first.";
    return false;
  }
  if (!audio.startRecording()) {
    error = "Not enough memory to record. Enable PSRAM in the board settings.";
    return false;
  }

  _job++;
  _source = source;
  _agentKey = agentKey;
  _heard = "";
  _reply = "";
  _error = "";
  _recordedMs = 0;
  _phase = VoicePhase::Recording;
  feedback.tick();
  Serial.print("Voice: recording (");
  Serial.print(voiceSourceName(source));
  Serial.println(")");
  return true;
}

void VoiceFlow::onPress() {
  if (busy()) return;
  String error;
  if (!beginRecording(configStore.voice().agentKey, VoiceSource::Button, error)) fail(error);
}

bool VoiceFlow::startFromPage(const String &agentKey, String &error) {
  return beginRecording(agentKey, VoiceSource::Page, error);
}

bool VoiceFlow::stopFromPage(String &error) {
  if (_phase != VoicePhase::Recording || _source != VoiceSource::Page) {
    error = "The page mic is not recording.";
    return false;
  }
  finishRecording();
  return true;
}

bool VoiceFlow::speakWhenDone(uint32_t chatJob, const String &agentKey, String &error) {
  if (busy()) {
    error = "The desk is already in a voice exchange.";
    return false;
  }
  if (!configStore.voice().ttsReady()) {
    error = "Add the text-to-speech provider's API key under Settings › Voice.";
    return false;
  }
  _job++;
  _source = VoiceSource::Typed;
  _chatJob = chatJob;
  _agentKey = agentKey;
  _heard = "";
  _reply = "";
  _error = "";
  _recordedMs = 0;
  _speakAt = 0;
  _phase = VoicePhase::Waiting;
  return true;
}

void VoiceFlow::onRelease() {
  // The physical button only ends what the physical button started; a page
  // recording ends when the page says so or the length cap trips.
  if (_source == VoiceSource::Button) finishRecording();
}

void VoiceFlow::finishRecording() {
  if (_phase != VoicePhase::Recording) return;
  audio.pumpRecording();
  audio.stopRecording();
  _recordedMs = audio.recordedMs();
  feedback.tick();
  if (_recordedMs < RECORD_MIN_MS) {
    audio.discardRecording();
    fail("That was too short. Hold the button while you talk.");
    return;
  }
  sendRecording();
}

void VoiceFlow::sendRecording() {
  _phase = VoicePhase::Transcribing;
  feedback.follow(chatClient.phase(), true);
  Serial.print("Voice: transcribing ");
  Serial.print(audio.wavSize());
  Serial.println(" bytes");

  VoiceSettings voice = configStore.voice();
  String text;
  String error;
  bool ok = speech.transcribe(voice, audio.wav(), audio.wavSize(), "desk.wav", "audio/wav", text, error);
  audio.discardRecording();
  if (!ok) {
    fail(error);
    return;
  }
  if (!text.length()) {
    fail("I didn't catch that. Try again a little closer to the mic.");
    return;
  }
  _heard = text;
  Serial.print("Voice: heard \"");
  Serial.print(text);
  Serial.println("\"");

  AgentKind kind = ConfigStore::parseAgent(_agentKey);
  AgentSettings agent = configStore.agent(kind);
  if (!chatClient.start(ConfigStore::agentKey(kind), agent, text)) {
    fail("The desk is busy with another message.");
    return;
  }
  _chatJob = chatClient.status().job;
  _speakAt = 0;
  _phase = VoicePhase::Waiting;
}

void VoiceFlow::speakReply() {
  _phase = VoicePhase::Speaking;
  // Let the reply chime finish before the speaker starts.
  feedback.follow(chatClient.phase());
  Serial.println("Voice: speaking reply");
  VoiceSettings voice = configStore.voice();
  String error;
  if (!voice.ttsReady()) {
    _error = "Add the text-to-speech provider's API key under Settings › Voice to hear replies.";
  } else if (!speech.speak(voice, _reply, error)) {
    // The text reply still stands; only the read-out failed.
    _error = error;
    Serial.print("Voice: speech failed: ");
    Serial.println(error);
  }
  _phase = VoicePhase::Done;
}

void VoiceFlow::fail(const String &message, bool chime) {
  _error = message;
  _phase = VoicePhase::Failed;
  Serial.print("Voice: ");
  Serial.println(message);
  feedback.follow(chatClient.phase());
  if (chime) feedback.error();
}
