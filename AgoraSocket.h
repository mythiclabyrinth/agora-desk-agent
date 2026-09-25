#pragma once

#include <Arduino.h>

#include "Config.h"
#include "Json.h"

// Agora's UI socket, GET {url}/ws?token=…: the server pushes every event the
// user may see, so an agent's reply arrives the moment it is posted, without
// waiting for a REST poll. The desk sends nothing and reads only new
// messages.
//
// Closed -> open() -> Connecting -> handshake -> Connected. The token rides
// only in the handshake's query string: once connected, the link forgets the
// URL, so a dropped link goes back to Closed and needs open() again (from
// ChatClient, which holds the token for the job). Frames are never logged;
// they carry other people's messages.
class AgoraSocket {
 public:
  using MessageFn = void (*)(const JsonMessage &message, const String &channelId, void *ctx);

  // Starts connecting from update(), unless already open for this agent.
  // False when it cannot now (no Wi-Fi, bad URL, no token, too little heap
  // for TLS, or a drop too recent); the caller polls REST meanwhile.
  bool open(const AgentSettings &agent);
  // open() ahead of the job for plain ws, so the handshake is done before the
  // POST; wss waits for the job, as TLS needs the heap STT is using.
  void prepare(const AgentSettings &agent);
  void close();
  bool connected() const { return _state == State::Connected; }
  // Handshakes completed so far: a change means the link (re)connected.
  uint32_t sessions() const { return _sessions; }
  void update();
  // Called for each {"type":"message"} event while connected.
  void onMessage(MessageFn fn, void *ctx) {
    _fn = fn;
    _ctx = ctx;
  }

 private:
  enum class State : uint8_t { Closed, Connecting, Connected };

  // `type` is the library's WStype_t.
  void handleEvent(int type, const uint8_t *payload, size_t length);
  void halt();
  // Refuses open() for a while after a failure, so a caller retrying every
  // loop pass neither spins nor floods the log.
  bool holding() const;
  void hold(const char *why);

  State _state = State::Closed;
  String _identity;  // url + agent: what the open link belongs to (never the token)
  String _where;     // scheme://host:port, for the log
  uint32_t _sessions = 0;
  unsigned long _heldAt = 0;
  bool _held = false;
  bool _dropped = false;  // the last link fell, rather than being closed
  bool _failedQuietly = false;  // one "could not connect" line per open()
  bool _dispatching = false;
  bool _haltPending = false;
  MessageFn _fn = nullptr;
  void *_ctx = nullptr;
};

extern AgoraSocket agoraSocket;
