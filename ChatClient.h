#pragma once

#include <Arduino.h>
#include <HTTPClient.h>

#include "Config.h"
#include "Feedback.h"
#include "Json.h"
#include "ReplyWatch.h"

struct ListenStatus {
  Phase phase = Phase::Idle;
  uint32_t job = 0;
  String agentKey;
  String from;
  String text;
  String reply;
  String error;
  unsigned long waitedMs = 0;
  // millis() stamps for the latency trace, 0 until reached.
  uint32_t postStart = 0;
  uint32_t postDone = 0;
  uint32_t replySeen = 0;
};

struct HttpResponse {
  int status = -1;
  String body;
  String error;
  bool lowMemory = false;
  bool tooBig = false;  // body over the caller's cap; dropped
};

class ChatClient {
 public:
  bool start(const String &agentKey, const AgentSettings &agent, const String &text);
  void update();
  Phase phase() const;
  ListenStatus status() const;

  // GET {agent.url}{path} with the agent's bearer token. Blocks for the
  // request. maxBody 0 keeps any size.
  HttpResponse fetch(const AgentSettings &agent, const String &path, size_t maxBody);
  // The id Agora knows the agent by: agent_id, else the slug of the name.
  // Never the desk key.
  static String agoraId(const AgentSettings &agent);

 private:
  void postMessage();
  void pollReply();
  static void onSocketMessage(const JsonMessage &message, const String &channelId, void *ctx);
  void succeed(const String &reply, const char *via);
  void fail(const String &message);
  HttpResponse exchange(const String &token, bool post, const String &url, const String &payload,
                        size_t maxBody = 0);
  HttpResponse finishExchange(HTTPClient &http, const String &token, bool post, const String &payload,
                              size_t maxBody);
  String endpoint() const;
  String mentionText() const;
  void forgetToken();

  Phase _phase = Phase::Idle;
  uint32_t _job = 0;
  AgentSettings _agent;
  String _agentKey;
  String _text;
  String _reply;
  String _error;
  long long _sentId = 0;
  bool _posted = false;
  unsigned long _started = 0;
  unsigned long _nextPoll = 0;
  uint32_t _postStart = 0;
  uint32_t _postDone = 0;
  uint32_t _replySeen = 0;
  // The reply arrives on Agora's socket while it is up; REST polls cover
  // the gaps, plus one catch-up read per (re)connect after the POST.
  ReplyWatch _watch;
  uint32_t _seenSession = 0;
};

extern ChatClient chatClient;
