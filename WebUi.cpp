#include "WebUi.h"

#include "AgoraSocket.h"
#include "Board.h"
#include "ChatClient.h"
#include "Config.h"
#include "Display.h"
#include "Json.h"
#include "Page.h"
#include "Portal.h"
#include "Audio.h"
#include "Speech.h"
#include "VoiceFlow.h"
#include "Wake.h"

#include <esp_heap_caps.h>
#include <uri/UriBraces.h>

namespace {

// A browser clip is opus/aac, so a minute is well under this.
constexpr size_t CLIP_MAX_BYTES = 3u * 1024 * 1024;
constexpr size_t CLIP_FIRST_ALLOC = 256u * 1024;
// Hundreds of channels; past this the page falls back to typing the id.
constexpr size_t CHANNELS_MAX_BYTES = 24u * 1024;

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

// Model and voice names: Groq ids like canopylabs/orpheus-v1-english.
bool validModel(const String &value, size_t maxLen) {
  if (!value.length() || value.length() > maxLen) return false;
  for (unsigned i = 0; i < value.length(); i++) {
    char c = value[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '/' || c == ':';
    if (!ok) return false;
  }
  return true;
}

// The slider sends two decimals; the slack absorbs 0.99 arriving as 0.9900001.
bool validCutoff(float cutoff) {
  return cutoff >= WAKE_CUTOFF_MIN - 0.001f && cutoff <= WAKE_CUTOFF_MAX + 0.001f;
}

constexpr char kCutoffRange[] = "Wake cutoff must be between 0.50 and 0.99.";

const AgentKind kKinds[] = {AgentKind::Claude, AgentKind::Cursor, AgentKind::Codex};

// Only the channels the agent belongs to: the picker is for choosing where
// to reach it, and Agora lists the rest with member=false.
void appendChannel(const JsonItem &item, void *ctx) {
  auto *out = static_cast<String *>(ctx);
  if (!item.id.length() || !item.member) return;
  if (!out->endsWith("[")) *out += ',';
  *out += "{\"id\":\"";
  *out += jsonEscape(item.id);
  *out += "\",\"name\":\"";
  *out += jsonEscape(item.name);
  *out += "\",\"group\":\"";
  *out += jsonEscape(item.group);
  *out += "\",\"kind\":\"";
  *out += jsonEscape(item.kind);
  *out += "\"}";
}

}  // namespace

WebUi webUi;

void WebUi::begin() {
  _server.on("/", HTTP_GET, [this]() { handleIndex(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  _server.on("/api/listen", HTTP_GET, [this]() { handleListen(); });
  _server.on("/api/wifi", HTTP_POST, [this]() { handleWifi(); });
  _server.on("/api/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
  _server.on("/api/wifi/forget", HTTP_POST, [this]() { handleWifiForget(); });
  _server.on("/api/agents", HTTP_GET, [this]() { handleAgentsGet(); });
  _server.on("/api/agents", HTTP_POST, [this]() { handleAgentsPost(); });
  _server.on(UriBraces("/api/agents/{}/channels"), HTTP_POST, [this]() { handleAgentChannels(); });
  _server.on("/api/chat", HTTP_POST, [this]() { handleChat(); });
  _server.on("/api/voice", HTTP_GET, [this]() { handleVoiceGet(); });
  _server.on("/api/voice", HTTP_POST, [this]() { handleVoicePost(); });
  _server.on("/api/voice/keys", HTTP_POST, [this]() { handleVoiceKeys(); });
  _server.on("/api/voice/test", HTTP_POST, [this]() { handleVoiceTest(); });
  _server.on("/api/voice/talk", HTTP_POST, [this]() { handleVoiceTalk(); });
  _server.on("/api/voice/wake", HTTP_POST, [this]() { handleVoiceWake(); });
  _server.on("/api/voice/transcribe", HTTP_POST, [this]() { handleTranscribe(); },
             [this]() { handleTranscribeUpload(); });
  _server.on("/api/voice/say", HTTP_POST, [this]() { handleSay(); });
  _server.on("/api/voice/tone", HTTP_POST, [this]() { handleTone(); });

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

// The page ships gzipped (web/build.py); every browser inflates it, and it
// is a third of the flash and airtime of the plain HTML.
void WebUi::handleIndex() {
  _server.sendHeader("Cache-Control", "no-store");
  _server.sendHeader("Content-Encoding", "gzip");
  _server.send_P(200, "text/html; charset=utf-8", reinterpret_cast<PGM_P>(INDEX_HTML_GZ), INDEX_HTML_GZ_LEN);
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

  // The page mirrors what the button is doing so a spoken exchange shows up
  // in the chat log too.
  VoiceStatus voice = voiceFlow.status();
  VoiceSettings voiceSettings = configStore.voice();
  body += ",\"voice\":{\"state\":\"";
  body += voicePhaseName(voice.phase);
  body += "\",\"source\":\"";
  body += voiceSourceName(voice.source);
  body += "\",\"job\":";
  body += String(voice.job);
  body += ",\"agent\":\"";
  body += jsonEscape(voice.agentKey);
  // "agent" is this exchange's; target_agent is the hands-free setting the
  // next one will use, which the dial can change at any time.
  body += "\",\"target_agent\":\"";
  body += jsonEscape(voiceSettings.agentKey);
  body += "\",\"recorded_ms\":";
  body += String(voice.recordedMs);
  body += ",\"stt_ready\":";
  body += voiceSettings.sttReady() ? "true" : "false";
  body += ",\"tts_ready\":";
  body += voiceSettings.ttsReady() ? "true" : "false";
  body += ",\"mic\":";
  body += audio.micReady() ? "true" : "false";
  body += ",\"speaker\":";
  body += audio.ampReady() ? "true" : "false";
  body += ",\"wake\":";
  body += wakeJson();
  if (voice.heard.length()) {
    body += ",\"heard\":\"";
    body += jsonEscape(voice.heard);
    body += '"';
  }
  if (voice.phase == VoicePhase::Done) {
    body += ",\"reply\":\"";
    body += jsonEscape(voice.reply);
    body += '"';
  }
  body += ",\"cancelled\":";
  body += voice.cancelled ? "true" : "false";
  if (voice.error.length()) {
    body += ",\"error\":\"";
    body += jsonEscape(voice.error);
    body += '"';
  }
  body += '}';

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
  body += "],\"display\":{\"present\":";
  body += display.present() ? "true" : "false";
  body += "}}";
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

// Clears the saved network and password, then drops the link; the board is
// reachable on its setup network afterwards.
void WebUi::handleWifiForget() {
  if (voiceFlow.busy() || chatClient.phase() == Phase::Listening) {
    sendError(409, "Wait for the current message to finish.");
    return;
  }
  String ssid = configStore.wifiSsid();
  configStore.forgetWifi();
  portal.forget();
  String response = "{\"ok\":true,\"ap_ip\":\"";
  response += jsonEscape(portal.apIp());
  response += "\"}";
  sendJson(200, response);
  Serial.print("Wi-Fi forgotten: ");
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
  // The open link used the settings this save replaces; a listening job reopens it.
  agoraSocket.close();

  AgentSettings saved = configStore.agent(kind);
  String response = "{\"ok\":true,\"ready\":";
  response += saved.ready ? "true" : "false";
  response += '}';
  sendJson(200, response);
  Serial.print("Saved agent ");
  Serial.println(ConfigStore::agentKey(kind));
}

// {url?, agent_id?, name?, token?}: unsaved form values, so channels can be
// listed during first-time setup; empty fields fall back to the saved agent.
void WebUi::handleAgentChannels() {
  AgentKind kind = ConfigStore::parseAgent(_server.pathArg(0));
  if (kind == AgentKind::Unknown) {
    sendError(400, "Unknown agent.");
    return;
  }
  if (!portal.staConnected()) {
    sendError(400, "The board is not on Wi-Fi yet.");
    return;
  }
  String body = _server.arg("plain");
  AgentSettings agent = configStore.agent(kind);
  struct Field {
    const char *key;
    String *into;
  } fields[] = {{"url", &agent.url}, {"agent_id", &agent.agentId}, {"name", &agent.name}, {"token", &agent.token}};
  for (Field &f : fields) {
    String value;
    jsonTopString(body, f.key, value);
    value.trim();
    if (value.length()) *f.into = value;
  }
  if (!validUrl(agent.url)) {
    sendError(400, "Agora URL must look like http://192.168.1.20:4470, with no /api path.");
    return;
  }
  if (agent.agentId.length() && !validId(agent.agentId, 64)) {
    sendError(400, "Agent id can only use letters, numbers, dashes, and underscores.");
    return;
  }
  if (!validToken(agent.token)) {
    sendError(400, agent.token.length() ? "Access token looks wrong." : "Add the access token first.");
    return;
  }
  String agoraId = ChatClient::agoraId(agent);
  if (!validId(agoraId, 64)) {
    sendError(400, "Fill in the Agent ID first.");
    return;
  }

  HttpResponse response = chatClient.fetch(agent, "/api/agents/" + agoraId + "/channels", CHANNELS_MAX_BYTES);
  agent.token = "";
  if (response.lowMemory) {
    sendError(503, "The board is short on memory. Try again in a moment.");
    return;
  }
  if (response.tooBig) {
    sendError(502, "Too many channels to list here; type the ID.");
    return;
  }
  if (response.status <= 0) {
    String message = "Could not reach Agora at " + agent.url + ".";
    sendError(502, message.c_str());
    return;
  }
  if (response.status == 401 || response.status == 403) {
    sendError(502, "Agora rejected the access token.");
    return;
  }
  if (response.status == 404) {
    if (response.body.indexOf("Unknown agent") >= 0) {
      String message = "Agora has no agent with id “" + agoraId + "”. Check the Agent ID.";
      sendError(502, message.c_str());
    } else {
      sendError(502, "This Agora server cannot list channels yet. Type the channel ID instead.");
    }
    return;
  }
  if (response.status < 200 || response.status >= 300) {
    String message = "Agora could not list channels (" + String(response.status) + ").";
    sendError(502, message.c_str());
    return;
  }

  JsonItem header;
  jsonTopObject(response.body, "agent", header);
  String out;
  out.reserve(response.body.length() / 2 + 128);
  out += "{\"ok\":true,\"agent\":{\"id\":\"";
  out += jsonEscape(header.id.length() ? header.id : agoraId);
  out += "\",\"name\":\"";
  out += jsonEscape(header.name.length() ? header.name : agent.name);
  out += "\",\"live\":";
  out += header.live ? "true" : "false";
  out += "},\"channels\":[";
  if (!jsonEachObject(response.body, "channels", appendChannel, &out)) {
    sendError(502, "Agora sent something unexpected.");
    return;
  }
  response.body = String();
  out += "]}";
  sendJson(200, out);
}

void WebUi::handleVoiceGet() {
  VoiceSettings v = configStore.voice();
  String body;
  body.reserve(640);
  body += "{\"keys\":{\"groq\":";
  body += v.groqKey.length() ? "true" : "false";
  body += ",\"openai\":";
  body += v.openaiKey.length() ? "true" : "false";
  body += "},\"stt_provider\":\"";
  body += jsonEscape(v.sttProvider);
  body += "\",\"tts_provider\":\"";
  body += jsonEscape(v.ttsProvider);
  body += "\",\"stt_models\":{\"groq\":\"";
  body += jsonEscape(v.sttModelGroq);
  body += "\",\"openai\":\"";
  body += jsonEscape(v.sttModelOpenai);
  body += "\"},\"tts_models\":{\"groq\":\"";
  body += jsonEscape(v.ttsModelGroq);
  body += "\",\"openai\":\"";
  body += jsonEscape(v.ttsModelOpenai);
  body += "\"},\"tts_voices\":{\"groq\":\"";
  body += jsonEscape(v.voiceGroq);
  body += "\",\"openai\":\"";
  body += jsonEscape(v.voiceOpenai);
  body += "\"},\"accent\":\"";
  body += jsonEscape(v.accent);
  body += "\",\"stt_language\":\"";
  body += jsonEscape(v.sttLanguage);
  body += "\",\"agent\":\"";
  body += jsonEscape(v.agentKey);
  body += "\",\"stt_ready\":";
  body += v.sttReady() ? "true" : "false";
  body += ",\"tts_ready\":";
  body += v.ttsReady() ? "true" : "false";
  body += ",\"mic\":";
  body += audio.micReady() ? "true" : "false";
  body += ",\"speaker\":";
  body += audio.ampReady() ? "true" : "false";
  body += ",\"button_pin\":";
  body += String(TALK_BUTTON_PIN);
  const WakeSettings &wake = configStore.wake();
  body += ",\"wake_enabled\":";
  body += wake.enabled ? "true" : "false";
  body += ",\"wake_cutoff\":";
  body += String(wake.cutoff / 255.0f, 2);
  body += ",\"wake_available\":";
  body += wakeWord.available() ? "true" : "false";
  body += ",\"wake_phrase\":\"";
  body += jsonEscape(wakeWord.phrase());
  body += "\",\"listen_led_pin\":";
  body += String(LISTEN_LED_PIN);
  body += '}';
  sendJson(200, body);
}

// {enabled, available, armed, muted, cutoff, phrase, score, peak}: what the
// page needs to draw the wake-word section and the tuning meter.
String WebUi::wakeJson() {
  bool enabled = configStore.wake().enabled;
  String body = "{\"enabled\":";
  body += enabled ? "true" : "false";
  body += ",\"muted\":";
  body += enabled ? "false" : "true";
  body += ",\"available\":";
  body += wakeWord.available() ? "true" : "false";
  body += ",\"armed\":";
  body += voiceFlow.wakeArmed() ? "true" : "false";
  body += ",\"cutoff\":";
  body += String(configStore.wake().cutoff / 255.0f, 2);
  body += ",\"phrase\":\"";
  body += jsonEscape(wakeWord.phrase());
  body += "\",\"score\":";
  body += String(wakeWord.score(), 2);
  body += ",\"peak\":";
  body += String(wakeWord.peak(), 2);
  body += '}';
  return body;
}

// {enabled?, cutoff?}: the page's wake toggle (the same setting as the dial's
// long press) and cutoff slider. Allowed mid-exchange, unlike /api/voice.
void WebUi::handleVoiceWake() {
  String body = _server.arg("plain");
  bool enabled = false;
  float cutoff = 0;
  bool sentEnabled = jsonTopBool(body, "enabled", enabled);
  bool sentCutoff = jsonTopFloat(body, "cutoff", cutoff);
  if (sentCutoff && !validCutoff(cutoff)) {
    sendError(400, kCutoffRange);
    return;
  }
  if (sentCutoff) voiceFlow.setWakeCutoff(wakeCutoffByte(cutoff));
  String error;
  if (sentEnabled && !voiceFlow.setWakeEnabled(enabled, error)) {
    sendError(409, error.c_str());
    return;
  }
  String response = "{\"ok\":true,\"wake\":";
  response += wakeJson();
  response += '}';
  sendJson(200, response);
}

// Features: providers, models, voices, accent, STT language, and the hands-free agent. Keys
// are a separate call so this form never carries a secret. Every field is
// optional; the page sends "agent" only when the user picked one, so saving
// an accent never undoes a turn of the dial.
void WebUi::handleVoicePost() {
  if (voiceFlow.busy()) {
    sendError(409, "Wait for the current voice message to finish.");
    return;
  }
  String body = _server.arg("plain");
  VoiceSettings in = configStore.voice();
  struct Field {
    const char *key;
    String *into;
  } fields[] = {
      {"stt_provider", &in.sttProvider},     {"tts_provider", &in.ttsProvider},
      {"stt_model_groq", &in.sttModelGroq},  {"stt_model_openai", &in.sttModelOpenai},
      {"tts_model_groq", &in.ttsModelGroq},  {"tts_model_openai", &in.ttsModelOpenai},
      {"voice_groq", &in.voiceGroq},         {"voice_openai", &in.voiceOpenai},
      {"accent", &in.accent},                {"agent", &in.agentKey},
      {"stt_language", &in.sttLanguage},
  };
  for (Field &f : fields) {
    String value;
    if (jsonTopString(body, f.key, value)) {
      value.trim();
      *f.into = value;
    }
  }
  if (!voiceProviderKnown(in.sttProvider) || !voiceProviderKnown(in.ttsProvider)) {
    sendError(400, "Provider must be groq or openai.");
    return;
  }
  if (!validModel(in.sttModelGroq, 80) || !validModel(in.sttModelOpenai, 80) ||
      !validModel(in.ttsModelGroq, 80) || !validModel(in.ttsModelOpenai, 80)) {
    sendError(400, "Model names can only use letters, numbers, dashes, dots, and slashes.");
    return;
  }
  if (!validId(in.voiceGroq, 40) || !validId(in.voiceOpenai, 40)) {
    sendError(400, "Voice names use letters, numbers, dashes, and underscores.");
    return;
  }
  if (!voiceAccentKnown(in.accent)) {
    sendError(400, "Accent must be american, british, or arabic.");
    return;
  }
  in.sttLanguage.toLowerCase();
  if (!voiceLanguageValid(in.sttLanguage)) {
    sendError(400, "Language must be an ISO code such as en, or empty to auto-detect.");
    return;
  }
  if (ConfigStore::parseAgent(in.agentKey) == AgentKind::Unknown) {
    sendError(400, "Choose which agent the desk should reach hands-free.");
    return;
  }
  float wakeCutoff = 0;
  bool sentWakeCutoff = jsonTopFloat(body, "wake_cutoff", wakeCutoff);
  if (sentWakeCutoff && !validCutoff(wakeCutoff)) {
    sendError(400, kCutoffRange);
    return;
  }
  bool wakeOn = false;
  if (jsonTopBool(body, "wake_enabled", wakeOn)) {
    String error;
    if (!voiceFlow.setWakeEnabled(wakeOn, error)) {
      sendError(409, error.c_str());
      return;
    }
  }
  if (sentWakeCutoff) voiceFlow.setWakeCutoff(wakeCutoffByte(wakeCutoff));
  configStore.saveVoiceFeatures(in);
  VoiceSettings saved = configStore.voice();
  String response = "{\"ok\":true,\"stt_ready\":";
  response += saved.sttReady() ? "true" : "false";
  response += ",\"tts_ready\":";
  response += saved.ttsReady() ? "true" : "false";
  response += '}';
  sendJson(200, response);
  Serial.println("Saved voice settings");
}

// {provider, api_key} saves; {provider, clear:true} forgets. Write-only.
void WebUi::handleVoiceKeys() {
  if (voiceFlow.busy()) {
    sendError(409, "Wait for the current voice message to finish.");
    return;
  }
  String body = _server.arg("plain");
  String provider;
  String key;
  bool clear = false;
  jsonTopString(body, "provider", provider);
  jsonTopString(body, "api_key", key);
  jsonTopBool(body, "clear", clear);
  provider.trim();
  key.trim();
  if (!voiceProviderKnown(provider)) {
    sendError(400, "Provider must be groq or openai.");
    return;
  }
  if (!clear) {
    if (!validToken(key)) {
      sendError(400, "That does not look like an API key.");
      return;
    }
    if (provider == VOICE_GROQ && !key.startsWith("gsk_")) {
      sendError(400, "Groq keys start with gsk_.");
      return;
    }
    if (provider == VOICE_OPENAI && !key.startsWith("sk-")) {
      sendError(400, "OpenAI keys start with sk-.");
      return;
    }
  }
  configStore.saveVoiceKey(provider, clear ? String() : key);
  sendJson(200, "{\"ok\":true}");
  Serial.print(clear ? "Cleared " : "Saved ");
  Serial.print(provider);
  Serial.println(" key");
}

// Probe the saved key for a provider by listing models.
void WebUi::handleVoiceTest() {
  if (voiceFlow.busy()) {
    sendError(409, "Wait for the current voice message to finish.");
    return;
  }
  if (!portal.staConnected()) {
    sendError(400, "The board is not on Wi-Fi yet.");
    return;
  }
  String provider;
  jsonTopString(_server.arg("plain"), "provider", provider);
  provider.trim();
  if (!voiceProviderKnown(provider)) {
    sendError(400, "Provider must be groq or openai.");
    return;
  }
  VoiceSettings v = configStore.voice();
  const String &key = v.keyFor(provider);
  if (!key.length()) {
    sendError(400, "No key saved for that provider yet.");
    return;
  }
  String error;
  if (!speech.testKey(provider, key, error)) {
    sendError(502, error.c_str());
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

// The page's mic button: {action:"start", agent} opens the board mic for
// that agent; {action:"stop"} sends what was heard.
void WebUi::handleVoiceTalk() {
  String body = _server.arg("plain");
  String action;
  String agent;
  jsonTopString(body, "action", action);
  jsonTopString(body, "agent", agent);
  String error;
  bool ok;
  if (action == "start") {
    ok = voiceFlow.startFromPage(agent, error);
  } else if (action == "stop") {
    ok = voiceFlow.stopFromPage(error);
  } else {
    sendError(400, "Action must be start or stop.");
    return;
  }
  if (!ok) {
    sendError(409, error.c_str());
    return;
  }
  VoiceStatus s = voiceFlow.status();
  String response = "{\"ok\":true,\"job\":";
  response += String(s.job);
  response += ",\"state\":\"";
  response += voicePhaseName(s.phase);
  response += "\"}";
  sendJson(200, response);
}

void WebUi::dropClip() {
  if (_clip) free(_clip);
  _clip = nullptr;
  _clipLen = 0;
  _clipCap = 0;
  _clipTooBig = false;
  _clipName = "";
  _clipType = "";
}

// Multipart upload from the page's microphone, part name "file". Called per
// chunk while the request body streams in; handleTranscribe runs after.
void WebUi::handleTranscribeUpload() {
  HTTPUpload &up = _server.upload();
  if (up.status == UPLOAD_FILE_START) {
    dropClip();
    _clipName = up.filename;
    _clipType = up.type;
    return;
  }
  if (up.status == UPLOAD_FILE_ABORTED) {
    dropClip();
    return;
  }
  if (up.status != UPLOAD_FILE_WRITE || _clipTooBig || !up.currentSize) return;
  if (_clipLen + up.currentSize > _clipCap) {
    size_t next = _clipCap ? _clipCap * 2 : CLIP_FIRST_ALLOC;
    while (next < _clipLen + up.currentSize) next *= 2;
    if (next > CLIP_MAX_BYTES) {
      _clipTooBig = true;
      return;
    }
    uint8_t *grown = static_cast<uint8_t *>(
        psramFound() ? heap_caps_realloc(_clip, next, MALLOC_CAP_SPIRAM) : realloc(_clip, next));
    if (!grown) {
      _clipTooBig = true;
      return;
    }
    _clip = grown;
    _clipCap = next;
  }
  memcpy(_clip + _clipLen, up.buf, up.currentSize);
  _clipLen += up.currentSize;
}

void WebUi::handleTranscribe() {
  if (_clipTooBig) {
    dropClip();
    sendError(413, "That recording is too long for the board. Keep it under a minute.");
    return;
  }
  if (!_clip || !_clipLen) {
    dropClip();
    sendError(400, "No audio arrived. Try recording again.");
    return;
  }
  if (!portal.staConnected()) {
    dropClip();
    sendError(400, "The board is not on Wi-Fi yet.");
    return;
  }
  VoiceSettings voice = configStore.voice();
  if (!voice.sttReady()) {
    dropClip();
    sendError(400, "Add the speech-to-text provider's API key under Settings › Voice.");
    return;
  }
  // The API infers the codec from the extension; keep it to what it accepts.
  String name = _clipName;
  name.toLowerCase();
  const char *exts[] = {".webm", ".ogg", ".mp4", ".m4a", ".mp3", ".wav", ".flac", ".mpeg", ".mpga"};
  bool known = false;
  for (const char *ext : exts) known = known || name.endsWith(ext);
  if (!known) {
    name = "clip.";
    if (_clipType.indexOf("mp4") >= 0 || _clipType.indexOf("aac") >= 0) name += "mp4";
    else if (_clipType.indexOf("ogg") >= 0) name += "ogg";
    else if (_clipType.indexOf("wav") >= 0) name += "wav";
    else name += "webm";
  }
  String text;
  String error;
  bool ok = speech.transcribe(voice, _clip, _clipLen, name, _clipType, text, error);
  size_t bytes = _clipLen;
  dropClip();
  if (!ok) {
    sendError(502, error.c_str());
    return;
  }
  String body = "{\"ok\":true,\"text\":\"";
  body += jsonEscape(text);
  body += "\",\"bytes\":";
  body += String(bytes);
  body += '}';
  sendJson(200, body);
  Serial.print("Voice: browser clip heard \"");
  Serial.print(text);
  Serial.println("\"");
}

// {text} -> a finite WAV the page plays through its own speaker. The key
// stays on the board; the page only ever sees audio.
void WebUi::handleSay() {
  if (!portal.staConnected()) {
    sendError(400, "The board is not on Wi-Fi yet.");
    return;
  }
  String text;
  jsonTopString(_server.arg("plain"), "text", text);
  text.trim();
  if (!text.length()) {
    sendError(400, "Nothing to say.");
    return;
  }
  VoiceSettings voice = configStore.voice();
  if (!voice.ttsReady()) {
    sendError(400, "Add the text-to-speech provider's API key under Settings › Voice.");
    return;
  }
  WavClip clip;
  String error;
  if (!speech.synthesize(voice, text, clip, error)) {
    sendError(502, error.c_str());
    return;
  }
  _server.sendHeader("Cache-Control", "no-store");
  _server.setContentLength(clip.len);
  _server.send(200, "audio/wav", "");
  _server.sendContent(reinterpret_cast<const char *>(clip.data), clip.len);
  wavFree(clip);
}

// A second of 440 Hz through the speaker path, so the amp can be checked
// without a speech provider: a clean tone here and noise from speech points
// at the stream, not the I2S setup.
void WebUi::handleTone() {
  if (voiceFlow.busy()) {
    sendError(409, "Wait for the current voice message to finish.");
    return;
  }
  if (!audio.startPlayback(24000, 1, 16)) {
    sendError(409, "Speaker is not connected.");
    return;
  }
  int16_t block[480];  // 20 ms at 24 kHz
  float phase = 0;
  for (int n = 0; n < 50; n++) {
    for (size_t i = 0; i < sizeof(block) / sizeof(block[0]); i++) {
      block[i] = static_cast<int16_t>(6000 * sinf(phase));
      phase += 2 * PI * 440 / 24000;
      if (phase > 2 * PI) phase -= 2 * PI;
    }
    audio.play(reinterpret_cast<const uint8_t *>(block), sizeof(block));
  }
  audio.stopPlayback();
  sendJson(200, "{\"ok\":true}");
}

void WebUi::handleChat() {
  if (chatClient.phase() == Phase::Listening) {
    sendError(409, "Already listening for a reply.");
    return;
  }
  if (voiceFlow.busy()) {
    sendError(409, "The button is mid-conversation. Give it a moment.");
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
  // "speak": the page's speaker toggle is on, so read this reply aloud too.
  bool speak = false;
  String speakNote;
  jsonTopBool(body, "speak", speak);
  if (speak && !voiceFlow.speakWhenDone(current.job, ConfigStore::agentKey(kind), speakNote)) {
    Serial.print("Chat: reply will not be spoken: ");
    Serial.println(speakNote);
  }
  String response = "{\"ok\":true,\"state\":\"listening\",\"job\":";
  response += String(current.job);
  response += ",\"speaking\":";
  response += (speak && !speakNote.length()) ? "true" : "false";
  if (speakNote.length()) {
    response += ",\"speak_note\":\"";
    response += jsonEscape(speakNote);
    response += '"';
  }
  response += '}';
  sendJson(200, response);
}
