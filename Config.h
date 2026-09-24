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

class ConfigStore {
 public:
  void begin();

  String wifiSsid();
  String wifiPassword();
  void saveWifi(const String &ssid, const String &password);

  AgentSettings agent(AgentKind kind);
  bool saveAgent(AgentKind kind, const AgentSettings &incoming, bool keepToken);

  static AgentKind parseAgent(const String &id);
  static const char *agentKey(AgentKind kind);
  static const char *agentTitle(AgentKind kind);

 private:
  String fieldKey(AgentKind kind, const char *suffix);

  Preferences _prefs;
};

extern ConfigStore configStore;
