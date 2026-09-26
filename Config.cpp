#include "Config.h"

#include "Board.h"
#include "Wake.h"

namespace {

struct AgentDefaults {
  const char *key;
  const char *title;
  const char *name;
  const char *agentId;
};

// These match the bridges' own defaults (AGENT_NAME / AGENT_ID).
constexpr AgentDefaults kAgents[] = {
    {"claude", "Claude", "Claude", "claude-cli"},
    {"cursor", "Cursor", "Cursor", "cursor-cli"},
    {"codex", "Codex", "Codex", "codex-cli"},
};

// Model and voice defaults mirror Agora's config.rs; Groq is the default provider.
constexpr char DEFAULT_GROQ_STT[] = "whisper-large-v3-turbo";
constexpr char DEFAULT_OPENAI_STT[] = "gpt-4o-mini-transcribe";
constexpr char DEFAULT_GROQ_TTS[] = "canopylabs/orpheus-v1-english";
constexpr char GROQ_ARABIC_TTS[] = "canopylabs/orpheus-arabic-saudi";
constexpr char DEFAULT_OPENAI_TTS[] = "gpt-4o-mini-tts";
constexpr char DEFAULT_GROQ_VOICE[] = "autumn";
constexpr char GROQ_ARABIC_VOICE[] = "noura";
constexpr char DEFAULT_OPENAI_VOICE[] = "alloy";
constexpr char DEFAULT_ACCENT[] = "american";
constexpr char DEFAULT_STT_LANGUAGE[] = "en";
constexpr char DEFAULT_VOICE_AGENT[] = "claude";

String orDefault(const String &value, const char *fallback) {
  return value.length() ? value : String(fallback);
}

uint8_t clampCutoff(uint8_t cutoff) {
  return wakeCutoffByte(cutoff / 255.0f);
}

const AgentDefaults *defaultsFor(AgentKind kind) {
  switch (kind) {
    case AgentKind::Claude: return &kAgents[0];
    case AgentKind::Cursor: return &kAgents[1];
    case AgentKind::Codex: return &kAgents[2];
    default: return nullptr;
  }
}

}  // namespace

ConfigStore configStore;

void ConfigStore::begin() {
  _prefs.begin("esp32agent", false);
  _wake.enabled = _prefs.getBool("wake_on", false);
  _wake.cutoff = clampCutoff(_prefs.getUChar("wake_cut", WakeWord::modelCutoff()));
  _voiceAgentSaved = _prefs.getString("voice_ag", "");
  _voiceAgent = parseAgent(_voiceAgentSaved) == AgentKind::Unknown ? String(DEFAULT_VOICE_AGENT) : _voiceAgentSaved;
}

String ConfigStore::wifiSsid() {
  return _prefs.getString("ssid", "");
}

String ConfigStore::wifiPassword() {
  return _prefs.getString("pass", "");
}

void ConfigStore::saveWifi(const String &ssid, const String &password) {
  _prefs.putString("ssid", ssid.c_str());
  _prefs.putString("pass", password.c_str());
}

void ConfigStore::forgetWifi() {
  _prefs.remove("ssid");
  _prefs.remove("pass");
}

String ConfigStore::fieldKey(AgentKind kind, const char *suffix) {
  const AgentDefaults *defaults = defaultsFor(kind);
  String key = defaults ? defaults->key : "x";
  key += "_";
  key += suffix;
  return key;
}

AgentKind ConfigStore::parseAgent(const String &id) {
  if (id == "claude") return AgentKind::Claude;
  if (id == "cursor") return AgentKind::Cursor;
  if (id == "codex") return AgentKind::Codex;
  return AgentKind::Unknown;
}

const char *ConfigStore::agentKey(AgentKind kind) {
  const AgentDefaults *defaults = defaultsFor(kind);
  return defaults ? defaults->key : "";
}

const char *ConfigStore::agentTitle(AgentKind kind) {
  const AgentDefaults *defaults = defaultsFor(kind);
  return defaults ? defaults->title : "";
}

AgentSettings ConfigStore::agent(AgentKind kind) {
  AgentSettings settings;
  const AgentDefaults *defaults = defaultsFor(kind);
  if (!defaults) return settings;

  String flagKey = fieldKey(kind, "set");
  settings.saved = _prefs.getString(flagKey.c_str(), "") == "1";
  if (!settings.saved) {
    settings.name = defaults->name;
    settings.agentId = defaults->agentId;
    return settings;
  }

  settings.name = _prefs.getString(fieldKey(kind, "nm").c_str(), "");
  settings.agentId = _prefs.getString(fieldKey(kind, "aid").c_str(), "");
  settings.url = _prefs.getString(fieldKey(kind, "url").c_str(), "");
  settings.channel = _prefs.getString(fieldKey(kind, "ch").c_str(), "");
  settings.token = _prefs.getString(fieldKey(kind, "tok").c_str(), "");
  settings.ready = settings.name.length() && settings.url.length() &&
                   settings.channel.length() && settings.token.length();
  return settings;
}

bool ConfigStore::saveAgent(AgentKind kind, const AgentSettings &incoming, bool keepToken) {
  if (!defaultsFor(kind)) return false;
  if (!incoming.name.length() || !incoming.url.length() || !incoming.channel.length()) {
    return false;
  }

  String token = incoming.token;
  if (keepToken) token = _prefs.getString(fieldKey(kind, "tok").c_str(), "");
  if (!token.length()) return false;

  _prefs.putString(fieldKey(kind, "nm").c_str(), incoming.name.c_str());
  _prefs.putString(fieldKey(kind, "aid").c_str(), incoming.agentId.c_str());
  _prefs.putString(fieldKey(kind, "url").c_str(), incoming.url.c_str());
  _prefs.putString(fieldKey(kind, "ch").c_str(), incoming.channel.c_str());
  _prefs.putString(fieldKey(kind, "tok").c_str(), token.c_str());
  _prefs.putString(fieldKey(kind, "set").c_str(), "1");
  return true;
}

bool voiceProviderKnown(const String &provider) {
  return provider == VOICE_GROQ || provider == VOICE_OPENAI;
}

bool voiceAccentKnown(const String &accent) {
  return accent == "american" || accent == "british" || accent == "arabic";
}

bool voiceLanguageValid(const String &language) {
  if (!language.length()) return true;
  if (language.length() < 2 || language.length() > 8) return false;
  for (unsigned i = 0; i < language.length(); i++) {
    char c = language[i];
    if (!isalpha(static_cast<unsigned char>(c)) && c != '-') return false;
  }
  return true;
}

const String &VoiceSettings::keyFor(const String &provider) const {
  return provider == VOICE_OPENAI ? openaiKey : groqKey;
}

const String &VoiceSettings::sttKey() const { return keyFor(sttProvider); }
const String &VoiceSettings::ttsKey() const { return keyFor(ttsProvider); }
bool VoiceSettings::sttReady() const { return sttKey().length() > 0; }
bool VoiceSettings::ttsReady() const { return ttsKey().length() > 0; }

String VoiceSettings::sttModel() const {
  return sttProvider == VOICE_OPENAI ? sttModelOpenai : sttModelGroq;
}

String VoiceSettings::ttsModel() const {
  if (ttsProvider == VOICE_OPENAI) return ttsModelOpenai;
  if (accent == "arabic") return GROQ_ARABIC_TTS;
  return ttsModelGroq;
}

String VoiceSettings::ttsVoice() const {
  if (ttsProvider == VOICE_OPENAI) return voiceOpenai;
  // The English voices cannot speak the Arabic model; fall back like Agora.
  bool arabic = accent == "arabic" || ttsModelGroq.indexOf("arabic") >= 0;
  if (arabic) {
    const char *arabicVoices[] = {"abdullah", "fahad", "sultan", "lulwa", "noura", "aisha"};
    for (const char *v : arabicVoices) if (voiceGroq == v) return voiceGroq;
    return GROQ_ARABIC_VOICE;
  }
  return voiceGroq;
}

VoiceSettings ConfigStore::voice() {
  VoiceSettings s;
  s.groqKey = _prefs.getString("vk_groq", "");
  s.openaiKey = _prefs.getString("vk_openai", "");
  s.sttProvider = orDefault(_prefs.getString("v_sttp", ""), VOICE_GROQ);
  s.ttsProvider = orDefault(_prefs.getString("v_ttsp", ""), VOICE_GROQ);
  s.sttModelGroq = orDefault(_prefs.getString("v_stt_g", ""), DEFAULT_GROQ_STT);
  s.sttModelOpenai = orDefault(_prefs.getString("v_stt_o", ""), DEFAULT_OPENAI_STT);
  s.ttsModelGroq = orDefault(_prefs.getString("v_tts_g", ""), DEFAULT_GROQ_TTS);
  s.ttsModelOpenai = orDefault(_prefs.getString("v_tts_o", ""), DEFAULT_OPENAI_TTS);
  s.voiceGroq = orDefault(_prefs.getString("v_vc_g", ""), DEFAULT_GROQ_VOICE);
  s.voiceOpenai = orDefault(_prefs.getString("v_vc_o", ""), DEFAULT_OPENAI_VOICE);
  s.accent = orDefault(_prefs.getString("v_acc", ""), DEFAULT_ACCENT);
  // Saved empty means auto-detect, so only a missing key takes the default.
  s.sttLanguage = _prefs.isKey("v_lang") ? _prefs.getString("v_lang", "") : String(DEFAULT_STT_LANGUAGE);
  s.agentKey = _voiceAgent;
  if (!voiceProviderKnown(s.sttProvider)) s.sttProvider = VOICE_GROQ;
  if (!voiceProviderKnown(s.ttsProvider)) s.ttsProvider = VOICE_GROQ;
  if (!voiceAccentKnown(s.accent)) s.accent = DEFAULT_ACCENT;
  if (!voiceLanguageValid(s.sttLanguage)) s.sttLanguage = DEFAULT_STT_LANGUAGE;
  return s;
}

void ConfigStore::saveVoiceFeatures(const VoiceSettings &in) {
  _prefs.putString("v_sttp", voiceProviderKnown(in.sttProvider) ? in.sttProvider.c_str() : VOICE_GROQ);
  _prefs.putString("v_ttsp", voiceProviderKnown(in.ttsProvider) ? in.ttsProvider.c_str() : VOICE_GROQ);
  _prefs.putString("v_stt_g", orDefault(in.sttModelGroq, DEFAULT_GROQ_STT).c_str());
  _prefs.putString("v_stt_o", orDefault(in.sttModelOpenai, DEFAULT_OPENAI_STT).c_str());
  _prefs.putString("v_tts_g", orDefault(in.ttsModelGroq, DEFAULT_GROQ_TTS).c_str());
  _prefs.putString("v_tts_o", orDefault(in.ttsModelOpenai, DEFAULT_OPENAI_TTS).c_str());
  _prefs.putString("v_vc_g", orDefault(in.voiceGroq, DEFAULT_GROQ_VOICE).c_str());
  _prefs.putString("v_vc_o", orDefault(in.voiceOpenai, DEFAULT_OPENAI_VOICE).c_str());
  _prefs.putString("v_acc", voiceAccentKnown(in.accent) ? in.accent.c_str() : DEFAULT_ACCENT);
  _prefs.putString("v_lang", voiceLanguageValid(in.sttLanguage) ? in.sttLanguage.c_str() : DEFAULT_STT_LANGUAGE);
  saveVoiceAgent(parseAgent(in.agentKey) == AgentKind::Unknown ? String(DEFAULT_VOICE_AGENT) : in.agentKey);
}

bool ConfigStore::setVoiceAgent(const String &key) {
  if (parseAgent(key) == AgentKind::Unknown) return false;
  _voiceAgent = key;
  return true;
}

bool ConfigStore::saveVoiceAgent(const String &key) {
  if (!setVoiceAgent(key)) return false;
  if (key != _voiceAgentSaved) {
    _prefs.putString("voice_ag", key.c_str());
    _voiceAgentSaved = key;
  }
  return true;
}

bool ConfigStore::saveVoiceKey(const String &provider, const String &key) {
  if (!voiceProviderKnown(provider)) return false;
  const char *slot = provider == VOICE_OPENAI ? "vk_openai" : "vk_groq";
  if (!key.length()) {
    _prefs.remove(slot);
    return true;
  }
  _prefs.putString(slot, key.c_str());
  return true;
}

uint8_t wakeCutoffByte(float cutoff) {
  if (cutoff < WAKE_CUTOFF_MIN) cutoff = WAKE_CUTOFF_MIN;
  if (cutoff > WAKE_CUTOFF_MAX) cutoff = WAKE_CUTOFF_MAX;
  return static_cast<uint8_t>(cutoff * 255.0f + 0.5f);
}

void ConfigStore::saveWake(const WakeSettings &in) {
  uint8_t cutoff = clampCutoff(in.cutoff);
  if (in.enabled != _wake.enabled) _prefs.putBool("wake_on", in.enabled);
  if (cutoff != _wake.cutoff) _prefs.putUChar("wake_cut", cutoff);
  _wake.enabled = in.enabled;
  _wake.cutoff = cutoff;
}
