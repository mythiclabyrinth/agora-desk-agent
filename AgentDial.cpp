#include "AgentDial.h"

#include "Board.h"
#include "Config.h"
#include "Dial.h"
#include "Feedback.h"
#include "VoiceFlow.h"

namespace {

const AgentKind kOrder[DIAL_AGENT_COUNT] = {AgentKind::Claude, AgentKind::Cursor, AgentKind::Codex};

int indexOf(const String &key) {
  AgentKind kind = ConfigStore::parseAgent(key);
  for (int i = 0; i < DIAL_AGENT_COUNT; i++) {
    if (kOrder[i] == kind) return i;
  }
  return -1;
}

// Timers compared this way survive millis() wrapping.
bool due(unsigned long now, unsigned long at) {
  return static_cast<long>(now - at) >= 0;
}

}  // namespace

AgentDial agentDial;

void AgentDial::begin() {
  dial.begin();
}

void AgentDial::update() {
  unsigned long now = millis();
  int steps = dial.takeSteps();
  if (steps) turn(steps, now);

  int8_t press = dial.pollPress();
  if (press > 0) {
    _pressedAt = now ? now : 1;
  } else if (press < 0 && _pressedAt) {
    if (now - _pressedAt < DIAL_SHORT_PRESS_MS) {
      _cue = Cue::Position;
      _cueAt = now;
    }
    _pressedAt = 0;
  }

  // The open recording already has its agent; a beep would land in the clip
  // and an NVS write stalls both cores, so both wait for it to end.
  if (voiceFlow.phase() == VoicePhase::Recording) return;
  if (_cue != Cue::None && due(now, _cueAt)) playCue();
  if (_unsaved && due(now, _saveAt)) {
    _unsaved = false;
    configStore.saveVoiceAgent(configStore.voiceAgent());
    Serial.print("Dial: saved hands-free agent ");
    Serial.println(configStore.voiceAgent());
  }
}

void AgentDial::turn(int steps, unsigned long now) {
  bool ready[DIAL_AGENT_COUNT];
  int readyCount = 0;
  for (int i = 0; i < DIAL_AGENT_COUNT; i++) {
    ready[i] = configStore.agent(kOrder[i]).ready;
    if (ready[i]) readyCount++;
  }
  int current = indexOf(configStore.voiceAgent());
  int next = dialPick(ready, current, steps);
  _cueAt = now + DIAL_BEEP_REST_MS;
  if (next < 0) {
    _cue = Cue::NoneReady;
    return;
  }
  if (next != current) {
    configStore.setVoiceAgent(ConfigStore::agentKey(kOrder[next]));
    _spinChanged = true;
    _unsaved = true;
    Serial.print("Dial: hands-free agent is now ");
    Serial.println(configStore.voiceAgent());
  }
  // Every click pushes the flash write back; only the resting choice is saved.
  _saveAt = now + DIAL_SAVE_REST_MS;
  _cue = readyCount == 1 && !_spinChanged ? Cue::OnlyOne : Cue::Position;
}

void AgentDial::playCue() {
  Cue cue = _cue;
  _cue = Cue::None;
  _spinChanged = false;
  if (cue == Cue::OnlyOne) {
    // Nothing else to switch to: one bare tick, shorter than any position cue.
    feedback.tick();
    return;
  }
  int at = indexOf(configStore.voiceAgent());
  // No ready agents, or a press while the page left an unconfigured agent
  // selected: the talk button would fail, so say so the same way it would.
  if (cue == Cue::NoneReady || at < 0 || !configStore.agent(kOrder[at]).ready) {
    feedback.error();
    return;
  }
  feedback.agentCue(at + 1);
}
