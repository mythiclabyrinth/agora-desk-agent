#include "ChatClient.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <ctype.h>
#include <esp_heap_caps.h>

#include "AgoraSocket.h"
#include "Board.h"
#include "Json.h"

namespace {

struct ReplyPick {
  const AgentSettings *agent;
  long long afterId = 0;
  long long bestId = 0;
  String text;
  bool found = false;
};

String slugify(const String &name) {
  String out;
  bool dash = false;
  for (unsigned i = 0; i < name.length(); i++) {
    char c = name[i];
    if (isalnum((unsigned char)c)) {
      out += (char)tolower(c);
      dash = false;
    } else if (!dash && out.length()) {
      out += '-';
      dash = true;
    }
  }
  while (out.endsWith("-")) out.remove(out.length() - 1);
  return out;
}

void considerReply(const JsonMessage &message, void *ctx) {
  auto *pick = static_cast<ReplyPick *>(ctx);
  if (!message.hasId || message.id <= pick->afterId) return;
  if (!replyFromAgent(message, pick->agent->agentId, pick->agent->name)) return;
  // The first reply after our message, not a later follow-up that landed
  // in the same poll.
  if (pick->found && message.id >= pick->bestId) return;
  pick->found = true;
  pick->bestId = message.id;
  pick->text = message.hasText ? message.text : "";
}

String trimUrl(String url) {
  url.trim();
  while (url.endsWith("/")) url.remove(url.length() - 1);
  return url;
}

String clip(const String &body) {
  String text = body;
  text.replace("\r", " ");
  text.replace("\n", " ");
  if (text.length() > 160) {
    text.remove(160);
    text += "...";
  }
  return text;
}

// Collects a response body up to a cap. Refusing a write makes HTTPClient
// stop reading, so an oversized body never reaches the heap whole.
class CappedBody : public Stream {
 public:
  CappedBody(String &into, size_t cap) : _into(into), _cap(cap) {}
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t *data, size_t len) override {
    if (_into.length() + len > _cap) {
      over = true;
      return 0;
    }
    _into.concat(reinterpret_cast<const char *>(data), len);
    return len;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
  bool over = false;

 private:
  String &_into;
  size_t _cap;
};

}  // namespace

ChatClient chatClient;

bool ChatClient::start(const String &agentKey, const AgentSettings &agent, const String &text) {
  if (_phase == Phase::Listening) return false;
  _job++;
  _agent = agent;
  _agentKey = agentKey;
  _text = text;
  _reply = "";
  _error = "";
  _sentId = 0;
  _posted = false;
  _started = millis();
  _nextPoll = 0;
  _postStart = 0;
  _postDone = 0;
  _replySeen = 0;
  _watch.arm(_job, agent.channel, agent.agentId, agent.name);
  _phase = Phase::Listening;
  Serial.print("Listening for ");
  Serial.println(agent.name);
  // The link connects from agoraSocket.update(), which loop() runs after
  // this client's update(), so a handshake never holds up the POST.
  agoraSocket.onMessage(onSocketMessage, this);
  agoraSocket.open(_agent);
  _seenSession = agoraSocket.sessions();
  return true;
}

void ChatClient::update() {
  if (_phase != Phase::Listening) return;
  if (millis() - _started > LISTEN_TIMEOUT_MS) {
    fail("No reply yet. The agent may still be working in Agora.");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    fail("Wi-Fi dropped while listening.");
    return;
  }
  if (!_posted) {
    postMessage();
    return;
  }
  if (agoraSocket.connected()) {
    // A link that came up after the POST may have missed the reply: read once,
    // then only the slow safety read.
    if (agoraSocket.sessions() != _seenSession) {
      _seenSession = agoraSocket.sessions();
      _nextPoll = millis() + LISTEN_SAFETY_POLL_MS;
      pollReply();
    } else if (static_cast<long>(millis() - _nextPoll) >= 0) {
      _nextPoll = millis() + LISTEN_SAFETY_POLL_MS;
      pollReply();
    }
    return;
  }
  agoraSocket.open(_agent);
  if (static_cast<long>(millis() - _nextPoll) < 0) return;
  _nextPoll = millis() + LISTEN_POLL_MS;
  pollReply();
}

void ChatClient::onSocketMessage(const JsonMessage &message, const String &channelId, void *ctx) {
  auto *self = static_cast<ChatClient *>(ctx);
  if (self->_phase != Phase::Listening || !self->_watch.accepts(self->_job, message, channelId)) return;
  self->succeed(message.hasText && message.text.length() ? message.text : String("(The agent replied without text.)"),
                "socket");
}

Phase ChatClient::phase() const {
  return _phase;
}

ListenStatus ChatClient::status() const {
  ListenStatus current;
  current.phase = _phase;
  current.job = _job;
  current.from = _agent.name;
  current.text = _text;
  current.reply = _reply;
  current.error = _error;
  current.waitedMs = _started ? millis() - _started : 0;
  current.agentKey = _agentKey;
  current.postStart = _postStart;
  current.postDone = _postDone;
  current.replySeen = _replySeen;
  return current;
}

String ChatClient::endpoint() const {
  return trimUrl(_agent.url) + "/api/channels/" + _agent.channel + "/messages";
}

String ChatClient::agoraId(const AgentSettings &agent) {
  return agent.agentId.length() ? agent.agentId : slugify(agent.name);
}

String ChatClient::mentionText() const {
  String prefix = "@";
  prefix += agoraId(_agent);
  if (_text.startsWith(prefix)) return _text;
  prefix += " ";
  prefix += _text;
  return prefix;
}

void ChatClient::forgetToken() {
  _agent.token = "";
}

void ChatClient::succeed(const String &reply, const char *via) {
  _replySeen = millis();
  _reply = reply;
  if (_reply.length() > MAX_REPLY_CHARS) {
    _reply.remove(MAX_REPLY_CHARS);
    _reply += "\n\n[truncated]";
  }
  _phase = Phase::Done;
  _watch.clear();
  agoraSocket.close();
  forgetToken();
  Serial.printf("Reply from %s via %s (free internal heap %u)\n", _agent.name.c_str(), via,
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
}

void ChatClient::fail(const String &message) {
  _error = message;
  _phase = Phase::Failed;
  _watch.clear();
  agoraSocket.close();
  forgetToken();
  Serial.println(message);
}

HttpResponse ChatClient::finishExchange(HTTPClient &http, const String &token, bool post, const String &payload,
                                       size_t maxBody) {
  HttpResponse result;
  http.setTimeout(8000);
  http.setConnectTimeout(10000);
  String authorization = "Bearer ";
  authorization += token;
  http.addHeader("Authorization", authorization);
  http.addHeader("User-Agent", "Esp32Agent");
  if (post) http.addHeader("Content-Type", "application/json");

  int status = post ? http.POST(payload) : http.GET();
  result.status = status;
  if (status <= 0) {
    result.error = HTTPClient::errorToString(status);
  } else if (!maxBody) {
    result.body = http.getString();
  } else if (http.getSize() > (int)maxBody) {
    result.tooBig = true;
  } else {
    CappedBody sink(result.body, maxBody);
    http.writeToStream(&sink);
    if (sink.over) {
      result.tooBig = true;
      result.body = String();
    }
  }
  http.end();
  return result;
}

HttpResponse ChatClient::exchange(const String &token, bool post, const String &url, const String &payload,
                                  size_t maxBody) {
  // Keep the TCP client alive until HTTPClient::end(). Constructing the
  // TLS client only for https avoids the SSL buffer on ordinary LAN calls.
  if (url.startsWith("https://")) {
    WiFiClientSecure tls;
    // Desk board, not a browser: skip CA checks so a LAN or self-signed
    // Agora still answers. The token still travels only to that host.
    tls.setInsecure();
    HTTPClient http;
    if (!http.begin(tls, url)) {
      HttpResponse result;
      result.error = "Could not open the Agora URL.";
      return result;
    }
    return finishExchange(http, token, post, payload, maxBody);
  }

  WiFiClient plain;
  HTTPClient http;
  if (!http.begin(plain, url)) {
    HttpResponse result;
    result.error = "Could not open the Agora URL.";
    return result;
  }
  return finishExchange(http, token, post, payload, maxBody);
}

HttpResponse ChatClient::fetch(const AgentSettings &agent, const String &path, size_t maxBody) {
  if (ESP.getFreeHeap() < 40000) {
    HttpResponse result;
    result.lowMemory = true;
    result.error = "Not enough memory.";
    return result;
  }
  return exchange(agent.token, false, trimUrl(agent.url) + path, "", maxBody);
}

void ChatClient::postMessage() {
  if (ESP.getFreeHeap() < 40000) {
    fail("Not enough memory to send.");
    return;
  }

  String payload = "{\"text\":\"";
  payload += jsonEscape(mentionText());
  payload += "\"}";
  _postStart = millis();
  HttpResponse response = exchange(_agent.token, true, endpoint(), payload);
  _postDone = millis();
  if (response.status < 200 || response.status >= 300) {
    String message = "Agora rejected the message";
    if (response.status > 0) {
      message += " (";
      message += String(response.status);
      message += ")";
    }
    if (response.body.length()) {
      message += ": ";
      message += clip(response.body);
    } else if (response.error.length()) {
      message += ": ";
      message += response.error;
    }
    fail(message);
    return;
  }

  if (!jsonTopLong(response.body, "id", _sentId) || _sentId <= 0) {
    fail("Agora accepted the message, but its id could not be read.");
    return;
  }
  _posted = true;
  _watch.afterId = _sentId;
  // A link already up saw everything since the POST; only a later one needs a catch-up read.
  if (agoraSocket.connected()) _seenSession = agoraSocket.sessions();
  _nextPoll = millis() + LISTEN_POLL_MS;
  Serial.print("Posted ");
  Serial.println((long)_sentId);
}

void ChatClient::pollReply() {
  if (ESP.getFreeHeap() < 40000) {
    fail("Not enough memory to keep listening.");
    return;
  }

  HttpResponse response = exchange(_agent.token, false, endpoint() + "?limit=15", "");
  if (response.status < 200 || response.status >= 300) {
    Serial.print("Listen poll ");
    Serial.println(response.status);
    return;
  }

  ReplyPick pick;
  pick.agent = &_agent;
  pick.afterId = _sentId;
  if (!jsonEachMessage(response.body, considerReply, &pick) || !pick.found) return;

  if (!pick.text.length()) pick.text = "(The agent replied without text.)";
  succeed(pick.text, "poll");
}
