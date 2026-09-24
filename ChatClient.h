#pragma once

#include <Arduino.h>
#include <HTTPClient.h>

#include "Config.h"
#include "Feedback.h"

struct ListenStatus {
  Phase phase = Phase::Idle;
  uint32_t job = 0;
  String agentKey;
  String from;
  String text;
  String reply;
  String error;
  unsigned long waitedMs = 0;
};

class ChatClient {
 public:
  bool start(const String &agentKey, const AgentSettings &agent, const String &text);
  void update();
  Phase phase() const;
  ListenStatus status() const;

 private:
  struct HttpResponse {
    int status = -1;
    String body;
    String error;
  };

  void postMessage();
  void pollReply();
  void succeed(const String &reply);
  void fail(const String &message);
  HttpResponse exchange(bool post, const String &url, const String &payload);
  HttpResponse finishExchange(HTTPClient &http, bool post, const String &payload);
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
};

extern ChatClient chatClient;
