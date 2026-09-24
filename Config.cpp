#include "Config.h"

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
