#pragma once

#include <Arduino.h>
#include <Preferences.h>

enum class AgentKind { Claude, Cursor, Codex, Unknown };

struct AgentSettings {
  String name;
  String agentId;
  String url;
  String channel;
  String token;
  bool saved = false;
  bool ready = false;
};

// Voice follows Agora's model: speech-to-text and text-to-speech each pick
// a provider (Groq or OpenAI) independently, keys are per provider, and each
// provider remembers its own model/voice so switching back loses nothing.
constexpr char VOICE_GROQ[] = "groq";
constexpr char VOICE_OPENAI[] = "openai";

struct VoiceSettings {
  String groqKey;
  String openaiKey;
  String sttProvider;
  String ttsProvider;
  String sttModelGroq;
  String sttModelOpenai;
  String ttsModelGroq;
  String ttsModelOpenai;
  String voiceGroq;
  String voiceOpenai;
  String accent;    // american | british | arabic
  // Spoken language for transcription (ISO-639-1, "en"); empty auto-detects.
  String sttLanguage;
  // The hands-free agent (claude, cursor, codex): what the talk button, the
  // wake word and the dial reach. From ConfigStore::voiceAgent().
  String agentKey;

  bool sttReady() const;
  bool ttsReady() const;
  const String &sttKey() const;
  const String &ttsKey() const;
  String sttModel() const;
  // Groq + Arabic swaps in the Arabic Orpheus model, as Agora does.
  String ttsModel() const;
  String ttsVoice() const;
  const String &keyFor(const String &provider) const;
};

bool voiceProviderKnown(const String &provider);
bool voiceAccentKnown(const String &accent);
// Empty, or 2-8 letters and dashes.
bool voiceLanguageValid(const String &language);

// Wake-word listening. `enabled` is the single source of truth: the dial's
// long press and the page both flip it, and the blue LED follows it.
struct WakeSettings {
  bool enabled = false;
  // Detector cutoff in its own 0-255 unit; ConfigStore::begin starts it at the model's.
  uint8_t cutoff = 0;
};

// A 0-1 cutoff in the detector's unit, clamped to WAKE_CUTOFF_MIN..MAX (Board.h).
uint8_t wakeCutoffByte(float cutoff);

class ConfigStore {
 public:
  void begin();

  String wifiSsid();
  String wifiPassword();
  void saveWifi(const String &ssid, const String &password);
  void forgetWifi();

  AgentSettings agent(AgentKind kind);
  bool saveAgent(AgentKind kind, const AgentSettings &incoming, bool keepToken);

  VoiceSettings voice();
  // Also saves the hands-free agent (through saveVoiceAgent).
  void saveVoiceFeatures(const VoiceSettings &incoming);
  bool saveVoiceKey(const String &provider, const String &key);  // empty key clears

  // The hands-free agent, cached so the dial can change it per click and the
  // page and dial share one value; flash is written only by saveVoiceAgent.
  const String &voiceAgent() const { return _voiceAgent; }
  // RAM only, effective at once. False for an unknown key.
  bool setVoiceAgent(const String &key);
  // RAM and NVS; the NVS write is skipped when flash already holds `key`.
  bool saveVoiceAgent(const String &key);

  // Cached in RAM: loop() asks every pass, and NVS reads are not free.
  const WakeSettings &wake() const { return _wake; }
  void saveWake(const WakeSettings &incoming);

  static AgentKind parseAgent(const String &id);
  static const char *agentKey(AgentKind kind);
  static const char *agentTitle(AgentKind kind);

 private:
  String fieldKey(AgentKind kind, const char *suffix);

  Preferences _prefs;
  WakeSettings _wake;
  String _voiceAgent;       // what the desk uses now
  String _voiceAgentSaved;  // what NVS holds ("" if never saved)
};

extern ConfigStore configStore;
