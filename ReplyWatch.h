#pragma once

#include <Arduino.h>

#include "Json.h"

// Which message answers a chat job. Pure logic, shared by the REST poll and
// the socket, so both pick replies the same way (and host tests can run it).

// Agora resolves an @mention by exact agent id, else by display name; a
// reply is recognised the same way.
inline bool replyFromAgent(const JsonMessage &message, const String &agentId, const String &agentName) {
  if (message.authorType != "agent") return false;
  if (agentId.length()) return message.authorId.equalsIgnoreCase(agentId);
  return message.authorName.equalsIgnoreCase(agentName);
}

// The live half: one job's claim on socket events. Events for another job,
// another channel, or anything at or before the posted message are not its reply.
struct ReplyWatch {
  uint32_t job = 0;  // 0: no job is waiting
  String channel;
  long long afterId = 0;  // the posted message's id; 0 until the POST returns
  String agentId;
  String agentName;

  void arm(uint32_t forJob, const String &forChannel, const String &id, const String &name) {
    job = forJob;
    channel = forChannel;
    afterId = 0;
    agentId = id;
    agentName = name;
  }
  void clear() { job = 0; }

  bool accepts(uint32_t currentJob, const JsonMessage &message, const String &channelId) const {
    if (!job || job != currentJob || afterId <= 0) return false;
    if (channelId != channel) return false;
    if (!message.hasId || message.id <= afterId) return false;
    return replyFromAgent(message, agentId, agentName);
  }
};
