#include "VoiceFlow.h"

#include "AgoraSocket.h"
#include "Audio.h"
#include "Board.h"
#include "ChatClient.h"
#include "Feedback.h"
#include "Portal.h"
#include "Speech.h"
#include "Wake.h"
#include "WakeText.h"

namespace {

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

void VoiceFlow::begin() {
  _talk.begin(TALK_BUTTON_PIN);
  audio.begin();
  wakeWord.setCutoff(configStore.wake().cutoff);
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
  s.missed = _phase == VoicePhase::Failed && _missed;
  s.cancelled = _phase == VoicePhase::Idle && _cancelled;
  return s;
}

bool VoiceFlow::clickRecording() const {
  return _phase == VoicePhase::Recording && (_source == VoiceSource::Wake || _source == VoiceSource::Page);
}

void VoiceFlow::readButtons() {
  // A single click is known only once the double-click window has passed.
  if (_clicks.poll(millis()) == ClickCounter::Click::Single && clickRecording()) {
    Serial.print("Voice: talk button ends the ");
    Serial.print(voiceSourceName(_source));
    Serial.println(" recording");
    finishRecording();
  }
  int8_t talk = _talk.poll();
  if (talk > 0) onPress();
  else if (talk < 0) onRelease();
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
      // Only the page needs a moment to fetch the reply; wake and button speak at once.
      bool pageWaits = _source == VoiceSource::Page || _source == VoiceSource::Typed;
      if (pageWaits && !_speakAt) _speakAt = millis() + SPEAK_GRACE_MS;
      if (pageWaits && static_cast<long>(millis() - _speakAt) < 0) return;
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
  uint32_t triggeredAt = millis();
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
  AgentSettings agent = configStore.agent(kind);
  if (kind == AgentKind::Unknown || !agent.ready) {
    error = "Set up that agent under Settings › Agents first.";
    return false;
  }
  if (!audio.startRecording(preRollMs)) {
    error = "Not enough memory to record. Enable PSRAM in the board settings.";
    return false;
  }
  // The agent is known: a plain ws link finishes its handshake while the user
  // talks, off the path from end of speech to reply.
  agoraSocket.prepare(agent);

  _trace.reset();
  _trace.trigger = triggeredAt;
  _trace.recordStart = millis();
  _chatJob = 0;
  _job++;
  _source = source;
  _agentKey = agentKey;
  _heard = "";
  _reply = "";
  _error = "";
  _recordedMs = 0;
  setPhase(VoicePhase::Recording);
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
  // During a wake or page recording the button clicks: once sends, twice
  // cancels. Holding it never starts a recording of its own.
  if (clickRecording()) {
    if (_clicks.press(millis()) == ClickCounter::Click::Double) cancelRecording("talk button", true);
    return;
  }
  _clicks.reset();
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
  }
  return true;
}

void VoiceFlow::setWakeCutoff(uint8_t cutoff) {
  WakeSettings w = configStore.wake();
  w.cutoff = cutoff;
  configStore.saveWake(w);
  wakeWord.setCutoff(configStore.wake().cutoff);
}

// Mute means "stop listening now": it turns wake listening off and, if the
// mic is open for any reason, drops that recording too. Otherwise it toggles.
void VoiceFlow::toggleMute() {
  String error;
  if (_phase == VoicePhase::Recording) {
    audio.discardRecording();
    releaseSocket();
    _trace.reset();
    _cancelled = false;
    setPhase(VoicePhase::Idle);
    _recordedMs = 0;
    Serial.println("Voice: muted, recording discarded");
    setWakeEnabled(false, error);
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

// Arm the detector only when a wake would be welcome right now. Sounds the
// board makes deafen it separately (WakeWord::holdOff).
void VoiceFlow::updateWake() {
  bool arm = wakeListening() && idlePhase(_phase) && chatClient.phase() != Phase::Listening && !audio.playing() &&
             wakeReady();
  _wakeArmed = arm;
  wakeWord.setArmed(arm);

  // A tuning trace: quiet in a silent room, one line a second while the
  // model reacts to something.
  unsigned long now = millis();
  if (arm && now - _wakeTracedAt >= WAKE_TRACE_MS) {
    _wakeTracedAt = now;
    float peak = wakeWord.peak();
    if (peak >= WAKE_TRACE_MIN) {
      Serial.printf("Wake word: peak %.2f, score %.2f, cutoff %.2f\n", peak, wakeWord.score(), wakeWord.cutoff());
    }
  }

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
      Serial.printf("Voice: pause after speech (%lu ms of quiet), sending\n", silenceMs);
      finishRecording();
      break;
    case VadVerdict::NoSpeech:
      cancelRecording("no speech after the wake word");
      break;
    default:
      break;
  }
}

// A false wake or a change of mind: no error beeps, no chat log entry, and
// above all no silent clip sent to Whisper (it answers silence with
// "Thank you.").
void VoiceFlow::cancelRecording(const char *why, bool byUser) {
  audio.discardRecording();
  releaseSocket();
  _trace.reset();
  _cancelled = byUser;
  setPhase(VoicePhase::Idle);
  _recordedMs = 0;
  Serial.print("Voice: ");
  Serial.print(voiceSourceName(_source));
  Serial.print(" recording cancelled, ");
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
  _trace.reset();
  _trace.trigger = millis();
  _job++;
  _source = VoiceSource::Typed;
  _chatJob = chatJob;
  _agentKey = agentKey;
  _heard = "";
  _reply = "";
  _error = "";
  _recordedMs = 0;
  _speakAt = 0;
  setPhase(VoicePhase::Waiting);
  return true;
}

void VoiceFlow::onRelease() {
  if (_clicks.release(millis())) return;
  // The physical button only ends what the physical button started; a page
  // recording ends on the page's word, a click, or the length cap.
  if (_source == VoiceSource::Button) finishRecording();
}

void VoiceFlow::finishRecording() {
  if (_phase != VoicePhase::Recording) return;
  _trace.speechEnd = millis();
  audio.pumpRecording();
  VadStats vad = mic.vad();
  if (_source == VoiceSource::Wake && !wakeHeardSpeech(vad)) {
    cancelRecording("the VAD heard no speech");
    return;
  }
  if (_source == VoiceSource::Wake) {
    _trace.finalSilenceMs = (vad.frames - vad.lastLoudFrame) * MIC_BLOCK_MS;
    _trace.hasSilence = true;
  }
  audio.stopRecording();
  _recordedMs = audio.recordedMs();
  feedback.tick();
  if (_recordedMs < RECORD_MIN_MS) {
    audio.discardRecording();
    fail("That was too short. Hold the button while you talk.", true, true);
    return;
  }
  sendRecording();
}

void VoiceFlow::sendRecording() {
  setPhase(VoicePhase::Transcribing);
  Serial.print("Voice: transcribing ");
  Serial.print(audio.wavSize());
  Serial.println(" bytes");

  VoiceSettings voice = configStore.voice();
  String text;
  String error;
  _trace.sttStart = millis();
  bool ok = speech.transcribe(voice, audio.wav(), audio.wavSize(), "desk.wav", "audio/wav", text, error);
  _trace.sttDone = millis();
  audio.discardRecording();
  if (!ok) {
    fail(error);
    return;
  }
  if (!text.length()) {
    fail("I didn't catch that. Try again a little closer to the mic.", true, true);
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
      fail(message, false, true);
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
  // The spoken reply is the desk's signal; the page, or a desk that cannot speak, keeps the chime.
  bool desk = _source == VoiceSource::Wake || _source == VoiceSource::Button;
  if (desk && voice.ttsReady() && audio.ampReady()) feedback.quietNextSuccess();
  _speakAt = 0;
  setPhase(VoicePhase::Waiting);
}

void VoiceFlow::releaseSocket() {
  // A chat job still listening owns the link; otherwise it was only prepared.
  if (chatClient.phase() != Phase::Listening) agoraSocket.close();
}

void VoiceFlow::speakReply() {
  // Speech blocks loop() for seconds, long enough to miss heartbeats; the
  // job is done, so the link goes before the speaker starts.
  releaseSocket();
  setPhase(VoicePhase::Speaking);
  // Any reply chime finishes before the speaker starts.
  feedback.follow(chatClient.phase());
  Serial.println("Voice: speaking reply");
  VoiceSettings voice = configStore.voice();
  String error;
  if (!voice.ttsReady()) {
    _error = "Add the text-to-speech provider's API key under Settings › Voice to hear replies.";
  } else {
    bool spoke = speech.speak(voice, _reply, error);
    const SpeechTiming &t = speech.timing();
    _trace.ttsStart = t.request;
    _trace.ttsHeaders = t.headers;
    _trace.ttsFirstPcm = t.firstPcm;
    _trace.playStart = t.playStart;
    _trace.playEnd = t.playEnd;
    if (!spoke) {
      // The text reply still stands; only the read-out failed.
      _error = error;
      Serial.print("Voice: speech failed: ");
      Serial.println(error);
    }
  }
  endTrace();
  setPhase(VoicePhase::Done);
}

void VoiceFlow::endTrace() {
  if (!_trace.trigger) return;  // no exchange began (a refused start)
  ListenStatus chat = chatClient.status();
  if (_chatJob && chat.job == _chatJob) {
    _trace.postStart = chat.postStart;
    _trace.postDone = chat.postDone;
    _trace.replySeen = chat.replySeen;
  }
  _trace.print();
  _trace.reset();
}

void VoiceFlow::setPhase(VoicePhase phase) {
  _phase = phase;
  if (_onPhase) _onPhase();
}

void VoiceFlow::fail(const String &message, bool chime, bool missed) {
  _error = message;
  _missed = missed;
  releaseSocket();
  endTrace();
  setPhase(VoicePhase::Failed);
  Serial.print("Voice: ");
  Serial.println(message);
  feedback.follow(chatClient.phase());
  if (chime) feedback.error();
}
