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
  String agentKey;  // which agent the talk button reaches: claude, cursor, codex

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

// Wake-word listening. The persisted flag is the single source of truth: the
// GPIO 7 button and the page both flip it here, and the blue LED follows it.
struct WakeSettings {
  bool enabled = false;
  String sensitivity = "medium";  // low | medium | high
};

bool wakeSensitivityKnown(const String &level);

class ConfigStore {
 public:
  void begin();

  String wifiSsid();
  String wifiPassword();
  void saveWifi(const String &ssid, const String &password);

  AgentSettings agent(AgentKind kind);
  bool saveAgent(AgentKind kind, const AgentSettings &incoming, bool keepToken);

  VoiceSettings voice();
  void saveVoiceFeatures(const VoiceSettings &incoming);
  bool saveVoiceKey(const String &provider, const String &key);  // empty key clears

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
};

extern ConfigStore configStore;
