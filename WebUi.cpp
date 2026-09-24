#include "WebUi.h"

#include "Board.h"
#include "ChatClient.h"
#include "Config.h"
#include "Json.h"
#include "Page.h"
#include "Portal.h"

namespace {

const char *phaseName(Phase phase) {
  switch (phase) {
    case Phase::Listening: return "listening";
    case Phase::Done: return "done";
    case Phase::Failed: return "failed";
    default: return "idle";
  }
}

bool validName(const String &name) {
  if (!name.length() || name.length() > 40) return false;
  for (unsigned i = 0; i < name.length(); i++) {
    char c = name[i];
    if (c < 0x20 || c == '"' || c == '\\' || c == '@') return false;
  }
  return true;
}

bool validId(const String &value, size_t maxLen) {
  if (!value.length() || value.length() > maxLen) return false;
  for (unsigned i = 0; i < value.length(); i++) {
    char c = value[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_';
    if (!ok) return false;
  }
  return true;
}

bool validUrl(const String &url) {
  if (url.length() < 8 || url.length() > 180) return false;
  if (!(url.startsWith("http://") || url.startsWith("https://"))) return false;
  if (url.indexOf(' ') >= 0 || url.indexOf("/api/") >= 0) return false;
  return true;
}

bool validToken(const String &token) {
  if (token.length() < 4 || token.length() > 500) return false;
  for (unsigned i = 0; i < token.length(); i++) {
    char c = token[i];
    if (c <= ' ' || c == '"' || c == '\\') return false;
  }
  return true;
}

const AgentKind kKinds[] = {AgentKind::Claude, AgentKind::Cursor, AgentKind::Codex};

}  // namespace

WebUi webUi;

void WebUi::begin() {
  _server.on("/", HTTP_GET, [this]() { handleIndex(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  _server.on("/api/listen", HTTP_GET, [this]() { handleListen(); });
  _server.on("/api/wifi", HTTP_POST, [this]() { handleWifi(); });
  _server.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
  _server.on("/api/agents", HTTP_GET, [this]() { handleAgentsGet(); });
  _server.on("/api/agents", HTTP_POST, [this]() { handleAgentsPost(); });
  _server.on("/api/chat", HTTP_POST, [this]() { handleChat(); });

  const char *captive[] = {
      "/generate_204", "/gen_204", "/hotspot-detect.html", "/library/test/success.html",
      "/connecttest.txt", "/ncsi.txt", "/fwlink", "/success.txt",
  };
  for (const char *path : captive) {
    _server.on(path, HTTP_GET, [this]() { handleCaptive(); });
  }

  _server.onNotFound([this]() { handleNotFound(); });
  _server.begin();
  Serial.println("Page on port 80");
}

void WebUi::handle() {
  _server.handleClient();
}

void WebUi::sendJson(int code, const String &body) {
  _server.sendHeader("Cache-Control", "no-store");
  _server.send(code, "application/json", body);
}

void WebUi::sendError(int code, const char *message) {
  String body = "{\"ok\":false,\"error\":\"";
  body += jsonEscape(message);
  body += "\"}";
  sendJson(code, body);
}

void WebUi::handleIndex() {
  _server.sendHeader("Cache-Control", "no-store");
  _server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void WebUi::handleCaptive() {
  _server.sendHeader("Location", "/");
  _server.send(302, "text/plain", "");
}

void WebUi::handleNotFound() {
  if (!portal.staConnected()) {
    handleCaptive();
    return;
  }
  _server.send(404, "text/plain", "Not found");
}

void WebUi::handleStatus() {
  String body;
  body.reserve(512);
  body += "{\"wifi\":";
  body += portal.staConnected() ? "true" : "false";
  body += ",\"ssid\":\"";
  body += jsonEscape(configStore.wifiSsid());
  body += "\",\"ip\":\"";
  body += jsonEscape(portal.staIp());
  body += "\",\"ap_ip\":\"";
  body += jsonEscape(portal.apIp());
  body += "\",\"host\":\"";
  body += MDNS_HOST;
  body += ".local\",\"listening\":";
  body += chatClient.phase() == Phase::Listening ? "true" : "false";
  body += ",\"agents\":[";
  for (int i = 0; i < 3; i++) {
    if (i) body += ',';
    AgentSettings agent = configStore.agent(kKinds[i]);
    body += "{\"id\":\"";
    body += ConfigStore::agentKey(kKinds[i]);
    body += "\",\"name\":\"";
    body += jsonEscape(agent.name);
    body += "\",\"ready\":";
    body += agent.ready ? "true" : "false";
    body += '}';
  }
  body += "]}";
  sendJson(200, body);
}

void WebUi::handleListen() {
  ListenStatus current = chatClient.status();
  String body;
  body.reserve(128 + current.reply.length() + current.error.length());
  body += "{\"state\":\"";
  body += phaseName(current.phase);
  body += "\",\"job\":";
  body += String(current.job);
  body += ",\"agent\":\"";
  body += jsonEscape(current.agentKey);
  body += "\",\"from\":\"";
  body += jsonEscape(current.from);
  body += "\",\"waited_ms\":";
  body += String(current.waitedMs);
  if (current.phase == Phase::Done) {
    body += ",\"reply\":\"";
    body += jsonEscape(current.reply);
    body += "\"";
  } else if (current.phase == Phase::Failed) {
    body += ",\"error\":\"";
    body += jsonEscape(current.error);
    body += "\"";
  }
  body += '}';
  sendJson(200, body);
}

void WebUi::handleWifiScan() {
  sendJson(200, portal.scanJson());
}

void WebUi::handleWifi() {
  String body = _server.arg("plain");
  String ssid;
  String password;
  if (!jsonTopString(body, "ssid", ssid)) {
    sendError(400, "Network name is required.");
    return;
  }
  jsonTopString(body, "password", password);
  String security;
  jsonTopString(body, "security", security);
  if (!ssid.length() || ssid.length() > 32) {
    sendError(400, "Network name must be 1–32 characters.");
    return;
  }
  if (password.length() > 63) {
    sendError(400, "Password is too long.");
    return;
  }
  if (security != "open" && !password.length() && ssid == configStore.wifiSsid()) {
    password = configStore.wifiPassword();
  }

  configStore.saveWifi(ssid, password);
  bool connected = portal.join(ssid, password, WIFI_JOIN_WAIT_MS);
  String response = "{\"ok\":true,\"connected\":";
  response += connected ? "true" : "false";
  response += ",\"ip\":\"";
  response += jsonEscape(portal.staIp());
  response += "\"}";
  sendJson(200, response);
  Serial.print("Wi-Fi saved: ");
  Serial.println(ssid);
}

void WebUi::handleAgentsGet() {
  String body = "{\"agents\":[";
  for (int i = 0; i < 3; i++) {
    if (i) body += ',';
    AgentSettings agent = configStore.agent(kKinds[i]);
    body += "{\"id\":\"";
    body += ConfigStore::agentKey(kKinds[i]);
    body += "\",\"title\":\"";
    body += ConfigStore::agentTitle(kKinds[i]);
    body += "\",\"name\":\"";
    body += jsonEscape(agent.name);
    body += "\",\"agent_id\":\"";
    body += jsonEscape(agent.agentId);
    body += "\",\"url\":\"";
    body += jsonEscape(agent.url);
    body += "\",\"channel\":\"";
    body += jsonEscape(agent.channel);
    body += "\",\"token_set\":";
    body += agent.token.length() ? "true" : "false";
    body += ",\"ready\":";
    body += agent.ready ? "true" : "false";
    body += '}';
  }
  body += "]}";
  sendJson(200, body);
}

void WebUi::handleAgentsPost() {
  String body = _server.arg("plain");
  String id;
  String name;
  String agentId;
  String url;
  String channel;
  String token;
  if (!jsonTopString(body, "id", id)) {
    sendError(400, "Which agent is this for?");
    return;
  }
  AgentKind kind = ConfigStore::parseAgent(id);
  if (kind == AgentKind::Unknown) {
    sendError(400, "Unknown agent.");
    return;
  }

  jsonTopString(body, "name", name);
  jsonTopString(body, "agent_id", agentId);
  jsonTopString(body, "url", url);
  jsonTopString(body, "channel", channel);
  bool sentToken = jsonTopString(body, "token", token);
  name.trim();
  agentId.trim();
  url.trim();
  channel.trim();
  token.trim();

  if (!validName(name)) {
    sendError(400, "Name needs 1–40 characters, without @ or quotes.");
    return;
  }
  if (agentId.length() && !validId(agentId, 64)) {
    sendError(400, "Agent id can only use letters, numbers, dashes, and underscores.");
    return;
  }
  if (!validUrl(url)) {
    sendError(400, "Agora URL must look like http://192.168.1.20:4470, with no /api path.");
    return;
  }
  if (!validId(channel, 80)) {
    sendError(400, "Channel id can only use letters, numbers, dashes, and underscores.");
    return;
  }
  bool keepToken = !sentToken || !token.length();
  if (!keepToken && !validToken(token)) {
    sendError(400, "Access token looks wrong.");
    return;
  }

  AgentSettings incoming;
  incoming.name = name;
  incoming.agentId = agentId;
  incoming.url = url;
  incoming.channel = channel;
  incoming.token = token;
  if (!configStore.saveAgent(kind, incoming, keepToken)) {
    sendError(400, "Access token is required the first time you save this agent.");
    return;
  }

  AgentSettings saved = configStore.agent(kind);
  String response = "{\"ok\":true,\"ready\":";
  response += saved.ready ? "true" : "false";
  response += '}';
  sendJson(200, response);
  Serial.print("Saved agent ");
  Serial.println(ConfigStore::agentKey(kind));
}

void WebUi::handleChat() {
  if (chatClient.phase() == Phase::Listening) {
    sendError(409, "Already listening for a reply.");
    return;
  }
  if (!portal.staConnected()) {
    sendError(400, "The board is not on Wi-Fi yet.");
    return;
  }

  String body = _server.arg("plain");
  String id;
  String text;
  if (!jsonTopString(body, "agent", id) || !jsonTopString(body, "text", text)) {
    sendError(400, "Choose an agent and write a message.");
    return;
  }
  text.trim();
  AgentKind kind = ConfigStore::parseAgent(id);
  if (kind == AgentKind::Unknown) {
    sendError(400, "Unknown agent.");
    return;
  }
  if (!text.length()) {
    sendError(400, "Message is empty.");
    return;
  }
  if (text.length() > MAX_USER_CHARS) {
    sendError(400, "Message is too long for the board.");
    return;
  }

  AgentSettings agent = configStore.agent(kind);
  if (!agent.ready) {
    sendError(400, "Set up that agent in Settings first.");
    return;
  }

  if (!chatClient.start(ConfigStore::agentKey(kind), agent, text)) {
    sendError(409, "Already listening for a reply.");
    return;
  }

  ListenStatus current = chatClient.status();
  String response = "{\"ok\":true,\"state\":\"listening\",\"job\":";
  response += String(current.job);
  response += '}';
  sendJson(200, response);
}
