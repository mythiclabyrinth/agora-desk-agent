#include "VoiceFlow.h"

#include "Audio.h"
#include "Board.h"
#include "ChatClient.h"
#include "Feedback.h"
#include "Portal.h"
#include "Speech.h"
#include "Wake.h"
#include "WakeText.h"

namespace {

constexpr unsigned long DEBOUNCE_MS = 40;
constexpr unsigned long SPEAK_GRACE_MS = 1500;
constexpr unsigned long WAKE_READY_CHECK_MS = 1000;

bool idlePhase(VoicePhase phase) {
  return phase == VoicePhase::Idle || phase == VoicePhase::Done || phase == VoicePhase::Failed;
}

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
    case VoiceSource::Wake: return "wake";
    default: return "button";
  }
}

void DebouncedButton::begin(uint8_t pin) {
  _pin = pin;
  pinMode(pin, INPUT_PULLUP);
  _raw = digitalRead(pin) == LOW;
  _pressed = _raw;
  _changedAt = millis();
}

int8_t DebouncedButton::poll() {
  bool raw = digitalRead(_pin) == LOW;
  unsigned long now = millis();
  if (raw != _raw) {
    _raw = raw;
    _changedAt = now;
    return 0;
  }
  if (raw == _pressed || now - _changedAt < DEBOUNCE_MS) return 0;
  _pressed = raw;
  return _pressed ? 1 : -1;
}

void VoiceFlow::begin() {
  _talk.begin(TALK_BUTTON_PIN);
  _muteButton.begin(MUTE_BUTTON_PIN);
  audio.begin();
  wakeWord.setSensitivity(configStore.wake().sensitivity.c_str());
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

void VoiceFlow::readButtons() {
  int8_t talk = _talk.poll();
  if (talk > 0) onPress();
  else if (talk < 0) onRelease();
  if (_muteButton.poll() > 0) onMuteButton();
}

void VoiceFlow::update() {
  readButtons();
  updateWake();

  if (_phase == VoicePhase::Recording) {
    audio.pumpRecording();
    if (audio.recordingFull() || audio.recordedMs() >= RECORD_MAX_MS) {
      Serial.println("Voice: clip is at the limit, sending");
      finishRecording();
      return;
    }
    if (_source == VoiceSource::Wake) checkWakeEnd();
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

bool VoiceFlow::beginRecording(const String &agentKey, VoiceSource source, String &error,
                               unsigned long preRollMs) {
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
  if (!audio.startRecording(preRollMs)) {
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
  feedback.listenLed(true);
  if (source == VoiceSource::Wake) {
    feedback.wakeChime();
    // Judge speech only from after the chime (and its hangover) onward.
    _vadStarted = false;
    _vadFromFrame = mic.vad().frames + VAD_CHIME_SKIP_MS / MIC_BLOCK_MS;
  } else {
    feedback.tick();
  }
  Serial.print("Voice: recording (");
  Serial.print(voiceSourceName(source));
  Serial.println(")");
  return true;
}

void VoiceFlow::onPress() {
  // Pressing the talk button during a hands-free recording means "done, send it".
  if (_phase == VoicePhase::Recording && _source == VoiceSource::Wake) {
    Serial.println("Voice: talk button ends the wake recording");
    finishRecording();
    return;
  }
  if (busy()) return;
  String error;
  if (!beginRecording(configStore.voice().agentKey, VoiceSource::Button, error, MIC_PREROLL_MS)) fail(error);
}

bool VoiceFlow::startFromPage(const String &agentKey, String &error) {
  return beginRecording(agentKey, VoiceSource::Page, error, MIC_PREROLL_MS);
}

bool VoiceFlow::wakeListening() const {
  return configStore.wake().enabled && wakeWord.available();
}

bool VoiceFlow::setWakeEnabled(bool on, String &error) {
  if (on && !wakeWord.available()) {
    error = "The wake word could not start on this board (see the serial log). The talk button still works.";
    return false;
  }
  WakeSettings w = configStore.wake();
  if (w.enabled != on) {
    w.enabled = on;
    configStore.saveWake(w);
    Serial.println(on ? "Wake word: listening on" : "Wake word: listening off");
  }
  if (!on) {
    // Muting takes effect now, not on the next loop pass.
    _wakeArmed = false;
    wakeWord.setArmed(false);
    if (_phase != VoicePhase::Recording) feedback.listenLed(false);
  }
  return true;
}

void VoiceFlow::setWakeSensitivity(const String &level) {
  WakeSettings w = configStore.wake();
  w.sensitivity = level;
  configStore.saveWake(w);
  wakeWord.setSensitivity(configStore.wake().sensitivity.c_str());
}

// Mute means "stop listening now": it turns wake listening off and, if the
// mic is open for any reason, drops that recording too. Otherwise it toggles.
void VoiceFlow::onMuteButton() {
  String error;
  if (_phase == VoicePhase::Recording) {
    audio.discardRecording();
    _phase = VoicePhase::Idle;
    _recordedMs = 0;
    Serial.println("Voice: muted, recording discarded");
    setWakeEnabled(false, error);
    feedback.listenLed(false);
    feedback.follow(chatClient.phase());
    feedback.muteToggled(true);
    return;
  }
  bool unmute = !configStore.wake().enabled;
  if (!setWakeEnabled(unmute, error)) {
    // Unmuting a board whose wake engine failed: nothing to listen with.
    Serial.print("Wake word: ");
    Serial.println(error);
    feedback.error();
    return;
  }
  feedback.muteToggled(!unmute);
}

// Keys, agent and Wi-Fi: cheap enough once a second, not every loop pass.
bool VoiceFlow::wakeReady() {
  unsigned long now = millis();
  if (_wakeCheckedAt && now - _wakeCheckedAt < WAKE_READY_CHECK_MS) return _wakeReady;
  _wakeCheckedAt = now;
  VoiceSettings voice = configStore.voice();
  AgentKind kind = ConfigStore::parseAgent(voice.agentKey);
  _wakeReady = voice.sttReady() && portal.staConnected() && mic.ready() && kind != AgentKind::Unknown &&
               configStore.agent(kind).ready;
  return _wakeReady;
}

// Arm the detector only when a wake would be welcome right now. The speaker
// and buzzer deafen it on their own (WakeWord::holdOff), so a reply that says
// the phrase cannot wake the board.
void VoiceFlow::updateWake() {
  bool arm = wakeListening() && idlePhase(_phase) && chatClient.phase() != Phase::Listening && !audio.playing() &&
             wakeReady();
  _wakeArmed = arm;
  wakeWord.setArmed(arm);
  // The blue LED means "the mic is listening": armed for the wake word, or
  // recording for any reason (a talk-button clip lights it even when muted).
  // It follows the arming policy, not the short deaf windows after a beep,
  // so it does not flicker.
  feedback.listenLed(arm || _phase == VoicePhase::Recording);

  uint32_t samplePos = 0;
  // A detection that raced a disarm is dropped: the board has moved on.
  if (wakeWord.takeDetection(samplePos) && arm) onWake(samplePos);
}

void VoiceFlow::onWake(uint32_t samplePos) {
  Serial.print("Wake word: heard \"");
  Serial.print(wakeWord.phrase());
  Serial.println("\"");
  // Start the clip where the phrase ended, so a request said in one breath
  // ("Hey Jarvis what's next") keeps its first word.
  unsigned long sinceMs = (mic.position() - samplePos) * 1000UL / MIC_SAMPLE_RATE;
  String error;
  if (!beginRecording(configStore.voice().agentKey, VoiceSource::Wake, error, sinceMs)) fail(error);
}

bool VoiceFlow::wakeHeardSpeech(const VadStats &now) const {
  if (!_vadStarted) return false;
  return (now.speechFrames - _vadFrom.speechFrames) * MIC_BLOCK_MS >= VAD_MIN_SPEECH_MS;
}

// Hands-free recordings end themselves: a pause after speech sends, and
// silence after the chime gives up.
void VoiceFlow::checkWakeEnd() {
  VadStats now = mic.vad();
  if (!_vadStarted) {
    if (static_cast<int32_t>(now.frames - _vadFromFrame) < 0) return;
    _vadFrom = now;
    _vadStarted = true;
  }
  unsigned long elapsedMs = (now.frames - _vadFrom.frames) * MIC_BLOCK_MS + VAD_CHIME_SKIP_MS;
  unsigned long speechMs = (now.speechFrames - _vadFrom.speechFrames) * MIC_BLOCK_MS;
  unsigned long silenceMs = (now.frames - now.lastLoudFrame) * MIC_BLOCK_MS;
  switch (vadVerdict(elapsedMs, speechMs, silenceMs)) {
    case VadVerdict::Done:
      Serial.println("Voice: pause after speech, sending");
      finishRecording();
      break;
    case VadVerdict::NoSpeech:
      cancelWake("no speech after the wake word");
      break;
    default:
      break;
  }
}

// A false wake or a change of mind: no error beeps, no chat log entry, and
// above all no silent clip sent to Whisper (it answers silence with
// "Thank you.").
void VoiceFlow::cancelWake(const char *why) {
  audio.discardRecording();
  _phase = VoicePhase::Idle;
  _recordedMs = 0;
  Serial.print("Voice: wake recording cancelled, ");
  Serial.println(why);
  feedback.follow(chatClient.phase());
  feedback.cancel();
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
  if (_source == VoiceSource::Wake && !wakeHeardSpeech(mic.vad())) {
    cancelWake("the VAD heard no speech");
    return;
  }
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
  if (_source == VoiceSource::Wake) {
    // The clip starts at the detection, but the phrase's tail (or a repeat
    // of it) often lands in the transcript.
    size_t cut = wakePhraseEnd(text.c_str(), wakeWord.name());
    if (cut) text = text.substring(cut);
    text.trim();
    if (!text.length()) {
      String message = "I heard “";
      message += wakeWord.phrase();
      message += "” but nothing after it. Say it, wait for the two beeps, then speak.";
      fail(message, false);
      feedback.cancel();
      return;
    }
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
