#include "StatusScreen.h"

#include "AgentDial.h"
#include "Board.h"
#include "ChatClient.h"
#include "Config.h"
#include "Display.h"
#include "Portal.h"
#include "Screens.h"
#include "Wake.h"

namespace {

const AgentKind kAgents[] = {AgentKind::Claude, AgentKind::Cursor, AgentKind::Codex};

const char *titleOf(const String &key) {
  return ConfigStore::agentTitle(ConfigStore::parseAgent(key));
}

const char *sourceHint(VoiceSource source) {
  switch (source) {
    case VoiceSource::Button: return "btn";
    case VoiceSource::Wake: return "wake";
    case VoiceSource::Page: return "page";
    default: return "";
  }
}

}  // namespace

StatusScreen statusScreen;

void StatusScreen::begin() {
  if (!display.present()) return;
  ScreenLines boot = bootScreen();
  display.setMain(boot.top, boot.bottom, false);
  // Transcribing and Speaking block loop() as soon as they start; drawing on
  // the change itself gets them on screen first.
  voiceFlow.onPhaseChange([] { statusScreen.update(); });
}

void StatusScreen::update() {
  if (!display.present()) return;
  unsigned long now = millis();
  bool changed = !_started;
  if (!_started) {
    // Only Wi-Fi is announced at boot; the rest starts from how things are.
    _voicePhase = voiceFlow.phase();
    _chatPhase = chatClient.phase();
    _wakeOn = configStore.wake().enabled;
    _agent = configStore.voiceAgent();
    _cues = agentDial.cueCount();
  }
  if (checkWifi()) changed = true;
  // Chat before voice: it needs the voice phase from before this pass.
  if (checkChat(now)) changed = true;
  if (checkVoice()) changed = true;
  if (checkControls()) changed = true;
  _started = true;
  if (!changed && now - _shownAt < DISPLAY_REFRESH_MS) return;
  _shownAt = now;
  showMain(now);
}

void StatusScreen::notify(const ScreenLines &lines, unsigned long ms) {
  display.notify(lines.top, lines.bottom, ms);
}

bool StatusScreen::checkWifi() {
  bool up = portal.staConnected();
  if (_started && up == _wifi) return false;
  _wifi = up;
  _address = up ? portal.staIp() : portal.apIp();
  if (up) notify(wifiNotice(_address), NOTICE_WIFI_MS);
  else notify(setupNotice(AP_SSID, _address), NOTICE_SETUP_MS);
  return true;
}

bool StatusScreen::checkChat(unsigned long now) {
  Phase phase = chatClient.phase();
  if (phase == _chatPhase) return false;
  _chatPhase = phase;
  if (phase == Phase::Listening) {
    _waitSince = now;
    _chatAgent = titleOf(chatClient.status().agentKey);
    return true;
  }
  // A voice exchange announces its own ending, after the reply is spoken.
  if (_voicePhase == VoicePhase::Waiting) return true;
  ListenStatus chat = chatClient.status();
  if (phase == Phase::Done) notify(replyNotice(_chatAgent, chat.reply), NOTICE_REPLY_MS);
  else if (phase == Phase::Failed) notify(errorNotice(chat.error), NOTICE_ERROR_MS);
  return true;
}

bool StatusScreen::checkVoice() {
  VoicePhase phase = voiceFlow.phase();
  if (phase == _voicePhase) return false;
  _voicePhase = phase;
  if (phase == VoicePhase::Recording) {
    if (voiceFlow.status().source == VoiceSource::Wake) notify(wakeNotice(wakeWord.phrase()), NOTICE_WAKE_MS);
  } else if (phase == VoicePhase::Speaking) {
    _reply = lcdText(voiceFlow.status().reply, DISPLAY_TEXT_MAX);
  } else if (phase == VoicePhase::Done) {
    VoiceStatus s = voiceFlow.status();
    notify(replyNotice(titleOf(s.agentKey), s.reply), NOTICE_REPLY_MS);
  } else if (phase == VoicePhase::Failed) {
    VoiceStatus s = voiceFlow.status();
    if (s.missed) notify(missedNotice(), NOTICE_MISSED_MS);
    else notify(errorNotice(s.error), NOTICE_ERROR_MS);
  }
  return true;
}

bool StatusScreen::checkControls() {
  bool changed = false;
  bool wakeOn = configStore.wake().enabled;
  if (wakeOn != _wakeOn) {
    _wakeOn = wakeOn;
    notify(muteNotice(!wakeOn, wakeWord.phrase()), NOTICE_MUTE_MS);
    changed = true;
  }
  // A turn shows at once; the cue (knob at rest, or a press) shows it again.
  uint32_t cues = agentDial.cueCount();
  if (configStore.voiceAgent() != _agent || cues != _cues) {
    _agent = configStore.voiceAgent();
    _cues = cues;
    AgentKind current = ConfigStore::parseAgent(_agent);
    int ready = 0;
    int place = 0;
    for (AgentKind kind : kAgents) {
      if (!configStore.agent(kind).ready) continue;
      ready++;
      if (kind == current) place = ready;
    }
    notify(agentNotice(ConfigStore::agentTitle(current), place, ready), NOTICE_AGENT_MS);
    changed = true;
  }
  return changed;
}

void StatusScreen::showMain(unsigned long now) {
  DeskView v;
  const WakeSettings &wake = configStore.wake();
  v.agent = titleOf(configStore.voiceAgent());
  v.wifi = _wifi;
  v.address = _address;
  v.wakeAvailable = wakeWord.available();
  v.wakeEnabled = wake.enabled;
  v.wakeArmed = voiceFlow.wakeArmed();
  v.wakePhrase = wakeWord.phrase();
  v.sensitivity = wake.sensitivity.c_str();
  v.activityAgent = _chatAgent;
  switch (_voicePhase) {
    case VoicePhase::Recording: {
      VoiceStatus s = voiceFlow.status();
      v.activity = DeskActivity::Recording;
      v.source = sourceHint(s.source);
      v.elapsedMs = s.recordedMs;
      break;
    }
    case VoicePhase::Transcribing:
      v.activity = DeskActivity::Thinking;
      break;
    case VoicePhase::Speaking:
      v.activity = DeskActivity::Speaking;
      v.reply = _reply;
      break;
    default:
      // A voice exchange waits on the same chat job as a typed message.
      if (_voicePhase == VoicePhase::Waiting || _chatPhase == Phase::Listening) {
        v.activity = DeskActivity::Waiting;
        v.elapsedMs = now - _waitSince;
      }
      break;
  }
  ScreenLines s = mainScreen(v);
  display.setMain(s.top, s.bottom, v.activity == DeskActivity::None);
}
