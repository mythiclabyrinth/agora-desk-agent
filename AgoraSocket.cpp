#include "AgoraSocket.h"

#include <WebSocketsClient.h>
#include <WiFi.h>
#include <ctype.h>

#include "Board.h"

namespace {

// The library keeps the handshake URL, token included, for its own retries.
// This subclass can drop it once the link is up, and stop the client for good.
class Link : public WebSocketsClient {
 public:
  void forgetUrl() { _client.cUrl = String(); }
  void stop() {
    disconnect();
    _client.cUrl = String();
    _host = String();
    _port = 0;  // loop() is a no-op from here until the next begin
  }
};

Link wsLink;

struct WsTarget {
  bool tls = false;
  String host;
  uint16_t port = 0;
  String prefix;  // path under which Agora is served, without a trailing slash
};

bool parseAgoraUrl(const String &raw, WsTarget &out) {
  String url = raw;
  url.trim();
  int from;
  if (url.startsWith("https://")) {
    out.tls = true;
    out.port = 443;
    from = 8;
  } else if (url.startsWith("http://")) {
    out.tls = false;
    out.port = 80;
    from = 7;
  } else {
    return false;
  }
  int slash = url.indexOf('/', from);
  String authority = slash < 0 ? url.substring(from) : url.substring(from, slash);
  out.prefix = slash < 0 ? String() : url.substring(slash);
  while (out.prefix.endsWith("/")) out.prefix.remove(out.prefix.length() - 1);
  int colon = authority.lastIndexOf(':');
  if (colon >= 0) {
    long port = authority.substring(colon + 1).toInt();
    if (port <= 0 || port > 65535) return false;
    out.port = static_cast<uint16_t>(port);
    authority.remove(colon);
  }
  out.host = authority;
  return out.host.length() > 0;
}

String queryEscape(const String &value) {
  static const char hex[] = "0123456789ABCDEF";
  String out;
  out.reserve(value.length() + 8);
  for (unsigned i = 0; i < value.length(); i++) {
    unsigned char c = static_cast<unsigned char>(value[i]);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += static_cast<char>(c);
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}

}  // namespace

AgoraSocket agoraSocket;

bool AgoraSocket::holding() const {
  return _held && millis() - _heldAt < AGORA_WS_RETRY_MS;
}

void AgoraSocket::hold(const char *why) {
  _held = true;
  _heldAt = millis();
  if (why) {
    Serial.print("Agora socket: ");
    Serial.println(why);
  }
}

bool AgoraSocket::open(const AgentSettings &agent) {
  String identity = agent.url;
  identity.trim();
  identity += '\n';
  identity += agent.agentId;
  identity += '\n';
  identity += agent.name;
  if (_state != State::Closed) {
    if (identity == _identity) return true;
    close();
  }
  if (holding() || WiFi.status() != WL_CONNECTED || !agent.token.length()) return false;

  WsTarget target;
  if (!parseAgoraUrl(agent.url, target)) {
    hold("the Agora URL cannot be reached as a socket; polling instead");
    return false;
  }
  if (target.tls && ESP.getFreeHeap() < AGORA_WSS_MIN_HEAP) {
    hold("not enough memory for a wss link; polling instead");
    return false;
  }

  bool again = _dropped && identity == _identity;
  _identity = identity;
  _held = false;
  _dropped = false;
  _failedQuietly = false;
  _haltPending = false;
  _where = target.tls ? "wss://" : "ws://";
  _where += target.host;
  _where += ':';
  _where += String(target.port);

  wsLink.onEvent([this](WStype_t type, uint8_t *payload, size_t length) { handleEvent(type, payload, length); });
  {
    // The only copy of the token made here; it is gone when this block ends.
    String path = target.prefix;
    path += "/ws?token=";
    path += queryEscape(agent.token);
    // No subprotocol: Agora does not ask for one.
    if (target.tls) wsLink.beginSSL(target.host.c_str(), target.port, path.c_str(), "", "");
    else wsLink.begin(target.host.c_str(), target.port, path.c_str(), "");
  }
  wsLink.setExtraHeaders("");  // no "Origin: file://"; the desk is not a browser
  wsLink.setReconnectInterval(AGORA_WS_RETRY_MS);
  wsLink.enableHeartbeat(AGORA_WS_PING_MS, AGORA_WS_PONG_MS, AGORA_WS_MISSED_PONGS);
  _state = State::Connecting;
  Serial.print(again ? "Agora socket: reconnecting to " : "Agora socket: connecting to ");
  Serial.println(_where);
  return true;
}

void AgoraSocket::prepare(const AgentSettings &agent) {
  String url = agent.url;
  url.trim();
  if (url.startsWith("http://")) open(agent);
}

void AgoraSocket::close() {
  bool wasOpen = _state != State::Closed;
  _state = State::Closed;
  _held = false;
  _dropped = false;
  if (_dispatching) {
    _haltPending = true;  // the library is mid-callback; stop it once loop() returns
  } else {
    halt();
  }
  if (wasOpen) {
    Serial.print("Agora socket: closed ");
    Serial.println(_where);
  }
}

void AgoraSocket::halt() {
  _haltPending = false;
  wsLink.stop();
}

void AgoraSocket::update() {
  if (_haltPending && !_dispatching) halt();
  if (_state == State::Closed) return;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Agora socket: Wi-Fi dropped");
    close();
    return;
  }
  _dispatching = true;
  wsLink.loop();
  _dispatching = false;
  if (_haltPending) halt();
}

void AgoraSocket::handleEvent(int type, const uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      // The payload is the handshake URL, token included: never print it.
      if (_state != State::Connecting) return;
      wsLink.forgetUrl();
      _state = State::Connected;
      _sessions++;
      Serial.print("Agora socket: connected to ");
      Serial.println(_where);
      break;
    case WStype_DISCONNECTED:
      if (_state == State::Connected) {
        // The URL is forgotten, so the library cannot retry; open() will.
        _state = State::Closed;
        _haltPending = true;
        _dropped = true;
        Serial.print("Agora socket: disconnected from ");
        Serial.println(_where);
        hold(nullptr);
      } else if (_state == State::Connecting && !_failedQuietly) {
        // A refused handshake says why ("HTTP 401"); the library retries.
        _failedQuietly = true;
        Serial.print("Agora socket: could not connect to ");
        Serial.print(_where);
        if (payload && length && length < 48) {
          Serial.print(" (");
          Serial.write(payload, length);
          Serial.print(')');
        }
        Serial.println(", retrying");
      }
      break;
    case WStype_TEXT: {
      if (_state != State::Connected || !_fn) return;
      String frame;
      frame.concat(reinterpret_cast<const char *>(payload), length);
      JsonMessage message;
      if (jsonEventMessage(frame, message)) _fn(message, message.channelId, _ctx);
      break;
    }
    default:
      break;
  }
}
