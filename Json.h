#pragma once

#include <Arduino.h>

// Just enough JSON for Agora bodies. The board only needs a few top-level
// fields, the messages array and the channel list, so this stays off
// ArduinoJson.

inline bool jsonIsWs(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

inline void jsonSkipWs(const String &s, int &i) {
  while (i < (int)s.length() && jsonIsWs(s[i])) i++;
}

inline bool jsonSkipString(const String &s, int &i) {
  if (i >= (int)s.length() || s[i] != '"') return false;
  i++;
  while (i < (int)s.length()) {
    char c = s[i++];
    if (c == '\\') {
      if (i < (int)s.length()) i++;
      continue;
    }
    if (c == '"') return true;
  }
  return false;
}

inline bool jsonSkipValue(const String &s, int &i) {
  jsonSkipWs(s, i);
  if (i >= (int)s.length()) return false;
  char c = s[i];
  if (c == '"') return jsonSkipString(s, i);
  if (c == '{' || c == '[') {
    char open = c;
    char close = (c == '{') ? '}' : ']';
    int depth = 1;
    i++;
    while (i < (int)s.length() && depth > 0) {
      char d = s[i];
      if (d == '"') {
        if (!jsonSkipString(s, i)) return false;
        continue;
      }
      i++;
      if (d == open) depth++;
      else if (d == close) depth--;
    }
    return depth == 0;
  }
  if (c == 't' || c == 'f' || c == 'n' || c == '-' || (c >= '0' && c <= '9')) {
    while (i < (int)s.length()) {
      char d = s[i];
      if (d == ',' || d == '}' || d == ']' || jsonIsWs(d)) break;
      i++;
    }
    return true;
  }
  return false;
}

inline void jsonAppendCodepoint(String &out, unsigned long cp) {
  if (cp < 0x80) {
    out += (char)cp;
  } else if (cp < 0x800) {
    out += (char)(0xC0 | (cp >> 6));
    out += (char)(0x80 | (cp & 0x3F));
  } else if (cp <= 0xFFFF) {
    out += (char)(0xE0 | (cp >> 12));
    out += (char)(0x80 | ((cp >> 6) & 0x3F));
    out += (char)(0x80 | (cp & 0x3F));
  } else {
    out += '?';
  }
}

inline bool jsonParseString(const String &s, int &i, String &out) {
  int start = i;
  if (!jsonSkipString(s, i)) return false;
  out = "";
  out.reserve((size_t)(i - start));
  for (int k = start + 1; k < i - 1; k++) {
    if (s[k] != '\\' || k + 1 >= i - 1) {
      out += s[k];
      continue;
    }
    char n = s[++k];
    switch (n) {
      case 'n': out += '\n'; break;
      case 'r': out += '\r'; break;
      case 't': out += '\t'; break;
      case '"':
      case '\\':
      case '/':
        out += n;
        break;
      case 'u': {
        unsigned long cp = 0;
        bool ok = true;
        for (int h = 1; h <= 4; h++) {
          if (k + h >= i - 1) {
            ok = false;
            break;
          }
          char hex = s[k + h];
          cp <<= 4;
          if (hex >= '0' && hex <= '9') cp += (unsigned long)(hex - '0');
          else if (hex >= 'a' && hex <= 'f') cp += (unsigned long)(hex - 'a' + 10);
          else if (hex >= 'A' && hex <= 'F') cp += (unsigned long)(hex - 'A' + 10);
          else ok = false;
        }
        k += 4;
        if (ok) jsonAppendCodepoint(out, cp);
        else out += '?';
        break;
      }
      default:
        out += n;
        break;
    }
  }
  return true;
}

inline String jsonEscape(const String &in) {
  String out;
  out.reserve(in.length() + 8);
  for (unsigned i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((unsigned char)c < 0x20) {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
          out += buf;
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

// onField may parse the value and advance `i`. If it leaves `i` unchanged
// the value is skipped. Return true from onField to stop the walk.
using JsonFieldFn = bool (*)(const String &key, const String &json, int &i, void *ctx);

inline bool jsonForEachTopField(const String &json, JsonFieldFn fn, void *ctx) {
  int i = 0;
  jsonSkipWs(json, i);
  if (i >= (int)json.length() || json[i] != '{') return false;
  i++;
  while (true) {
    jsonSkipWs(json, i);
    if (i >= (int)json.length()) return false;
    if (json[i] == '}') return true;
    String key;
    if (!jsonParseString(json, i, key)) return false;
    jsonSkipWs(json, i);
    if (i >= (int)json.length() || json[i] != ':') return false;
    i++;
    jsonSkipWs(json, i);
    int at = i;
    bool stop = fn(key, json, i, ctx);
    if (i == at && !jsonSkipValue(json, i)) return false;
    if (stop) return true;
    jsonSkipWs(json, i);
    if (i < (int)json.length() && json[i] == ',') {
      i++;
      continue;
    }
    return i < (int)json.length() && json[i] == '}';
  }
}

struct JsonTopString {
  const char *key;
  String value;
  bool found = false;
};

inline bool jsonTakeString(const String &key, const String &json, int &i, void *ctx) {
  auto *hit = static_cast<JsonTopString *>(ctx);
  if (key != hit->key) return false;
  hit->found = jsonParseString(json, i, hit->value);
  return true;
}

inline bool jsonTopString(const String &json, const char *key, String &out) {
  JsonTopString hit;
  hit.key = key;
  if (!jsonForEachTopField(json, jsonTakeString, &hit) || !hit.found) return false;
  out = hit.value;
  return true;
}

struct JsonTopLong {
  const char *key;
  long long value = 0;
  bool found = false;
};

inline bool jsonTakeLong(const String &key, const String &json, int &i, void *ctx) {
  auto *hit = static_cast<JsonTopLong *>(ctx);
  if (key != hit->key) return false;
  jsonSkipWs(json, i);
  if (i >= (int)json.length()) return true;
  char c = json[i];
  if (!(c == '-' || (c >= '0' && c <= '9'))) return true;
  hit->value = strtoll(json.c_str() + i, nullptr, 10);
  hit->found = true;
  while (i < (int)json.length()) {
    char d = json[i];
    if (d == ',' || d == '}' || d == ']' || jsonIsWs(d)) break;
    i++;
  }
  return true;
}

inline bool jsonTopLong(const String &json, const char *key, long long &out) {
  JsonTopLong hit;
  hit.key = key;
  if (!jsonForEachTopField(json, jsonTakeLong, &hit) || !hit.found) return false;
  out = hit.value;
  return true;
}

struct JsonTopFloat {
  const char *key;
  float value = 0;
  bool found = false;
};

inline bool jsonTakeFloat(const String &key, const String &json, int &i, void *ctx) {
  auto *hit = static_cast<JsonTopFloat *>(ctx);
  if (key != hit->key) return false;
  jsonSkipWs(json, i);
  if (i >= (int)json.length()) return true;
  char c = json[i];
  if (!(c == '-' || (c >= '0' && c <= '9'))) return true;
  char *end = nullptr;
  hit->value = strtof(json.c_str() + i, &end);
  hit->found = end != json.c_str() + i;
  i = end - json.c_str();
  return true;
}

inline bool jsonTopFloat(const String &json, const char *key, float &out) {
  JsonTopFloat hit;
  hit.key = key;
  if (!jsonForEachTopField(json, jsonTakeFloat, &hit) || !hit.found) return false;
  out = hit.value;
  return true;
}

struct JsonTopBool {
  const char *key;
  bool value = false;
  bool found = false;
};

inline bool jsonTakeBool(const String &key, const String &json, int &i, void *ctx) {
  auto *hit = static_cast<JsonTopBool *>(ctx);
  if (key != hit->key) return false;
  jsonSkipWs(json, i);
  if (json.startsWith("true", i)) {
    hit->value = true;
    hit->found = true;
    i += 4;
  } else if (json.startsWith("false", i)) {
    hit->value = false;
    hit->found = true;
    i += 5;
  }
  return true;
}

inline bool jsonTopBool(const String &json, const char *key, bool &out) {
  JsonTopBool hit;
  hit.key = key;
  if (!jsonForEachTopField(json, jsonTakeBool, &hit) || !hit.found) return false;
  out = hit.value;
  return true;
}

struct JsonMessage {
  long long id = 0;
  bool hasId = false;
  String channelId;
  String authorType;
  String authorId;
  String authorName;
  String text;
  bool hasText = false;
};

inline bool jsonReadMessage(const String &json, int &i, JsonMessage &message) {
  jsonSkipWs(json, i);
  if (i >= (int)json.length() || json[i] != '{') return false;
  i++;
  while (true) {
    jsonSkipWs(json, i);
    if (i >= (int)json.length()) return false;
    if (json[i] == '}') {
      i++;
      return true;
    }
    String key;
    if (!jsonParseString(json, i, key)) return false;
    jsonSkipWs(json, i);
    if (i >= (int)json.length() || json[i] != ':') return false;
    i++;
    jsonSkipWs(json, i);
    if (key == "id") {
      char c = (i < (int)json.length()) ? json[i] : 0;
      if (c == '-' || (c >= '0' && c <= '9')) {
        message.id = strtoll(json.c_str() + i, nullptr, 10);
        message.hasId = true;
      }
      if (!jsonSkipValue(json, i)) return false;
    } else if (key == "author_type" || key == "author_id" || key == "author_name" || key == "text" ||
               key == "channel_id") {
      if (i < (int)json.length() && json[i] == '"') {
        String value;
        if (!jsonParseString(json, i, value)) return false;
        if (key == "channel_id") message.channelId = value;
        else if (key == "author_type") message.authorType = value;
        else if (key == "author_id") message.authorId = value;
        else if (key == "author_name") message.authorName = value;
        else {
          message.text = value;
          message.hasText = true;
        }
      } else if (!jsonSkipValue(json, i)) {
        return false;
      }
    } else if (!jsonSkipValue(json, i)) {
      return false;
    }
    jsonSkipWs(json, i);
    if (i < (int)json.length() && json[i] == ',') i++;
  }
}

using JsonMessageFn = void (*)(const JsonMessage &message, void *ctx);

struct JsonMessageWalk {
  JsonMessageFn fn;
  void *ctx;
  bool saw = false;
};

inline bool jsonTakeMessages(const String &key, const String &json, int &i, void *ctx) {
  if (key != "messages") return false;
  auto *walk = static_cast<JsonMessageWalk *>(ctx);
  jsonSkipWs(json, i);
  if (i >= (int)json.length() || json[i] != '[') return true;
  i++;
  while (true) {
    jsonSkipWs(json, i);
    if (i >= (int)json.length()) return true;
    if (json[i] == ']') {
      i++;
      walk->saw = true;
      return true;
    }
    JsonMessage message;
    if (!jsonReadMessage(json, i, message)) return true;
    walk->fn(message, walk->ctx);
    jsonSkipWs(json, i);
    if (i < (int)json.length() && json[i] == ',') i++;
  }
}

inline bool jsonEachMessage(const String &json, JsonMessageFn fn, void *ctx) {
  JsonMessageWalk walk;
  walk.fn = fn;
  walk.ctx = ctx;
  jsonForEachTopField(json, jsonTakeMessages, &walk);
  return walk.saw;
}

// A frame from Agora's UI socket. Keys may come in any order (the server
// sorts them, so "message" precedes "type").
struct JsonEvent {
  String type;
  JsonMessage message;
  bool hasMessage = false;
};

inline bool jsonTakeEvent(const String &key, const String &json, int &i, void *ctx) {
  auto *event = static_cast<JsonEvent *>(ctx);
  if (key == "type" && i < (int)json.length() && json[i] == '"') {
    jsonParseString(json, i, event->type);
  } else if (key == "message") {
    int at = i;
    event->hasMessage = jsonReadMessage(json, i, event->message);
    if (!event->hasMessage) i = at;
  }
  return false;
}

// True for a new-message event ({"type":"message","message":{...}}); every
// other event type, and a malformed frame, is false.
inline bool jsonEventMessage(const String &json, JsonMessage &out) {
  JsonEvent event;
  if (!jsonForEachTopField(json, jsonTakeEvent, &event)) return false;
  if (event.type != "message" || !event.hasMessage) return false;
  out = event.message;
  return true;
}

// One flat Agora object: an entry of the channels array, or the agent header
// beside it. Fields it does not know, and non-scalar values, are skipped.
struct JsonItem {
  String id;
  String name;
  String group;
  String kind;
  bool member = false;
  bool live = false;
};

inline bool jsonReadItem(const String &json, int &i, JsonItem &item) {
  jsonSkipWs(json, i);
  if (i >= (int)json.length() || json[i] != '{') return false;
  i++;
  while (true) {
    jsonSkipWs(json, i);
    if (i >= (int)json.length()) return false;
    if (json[i] == '}') {
      i++;
      return true;
    }
    String key;
    if (!jsonParseString(json, i, key)) return false;
    jsonSkipWs(json, i);
    if (i >= (int)json.length() || json[i] != ':') return false;
    i++;
    jsonSkipWs(json, i);
    String *text = key == "id" ? &item.id
                   : key == "name" ? &item.name
                   : key == "group" ? &item.group
                   : key == "kind" ? &item.kind
                                   : nullptr;
    bool *flag = key == "member" ? &item.member : key == "live" ? &item.live : nullptr;
    if (text && i < (int)json.length() && json[i] == '"') {
      if (!jsonParseString(json, i, *text)) return false;
    } else if (flag && json.startsWith("true", i)) {
      *flag = true;
      i += 4;
    } else if (flag && json.startsWith("false", i)) {
      *flag = false;
      i += 5;
    } else if (!jsonSkipValue(json, i)) {
      return false;
    }
    jsonSkipWs(json, i);
    if (i < (int)json.length() && json[i] == ',') i++;
  }
}

using JsonItemFn = void (*)(const JsonItem &item, void *ctx);

struct JsonItemWalk {
  const char *key;
  JsonItemFn fn;
  void *ctx;
  bool saw = false;
};

inline bool jsonTakeItems(const String &key, const String &json, int &i, void *ctx) {
  auto *walk = static_cast<JsonItemWalk *>(ctx);
  if (key != walk->key) return false;
  jsonSkipWs(json, i);
  if (i >= (int)json.length() || json[i] != '[') return true;
  i++;
  while (true) {
    jsonSkipWs(json, i);
    if (i >= (int)json.length()) return true;
    if (json[i] == ']') {
      i++;
      walk->saw = true;
      return true;
    }
    JsonItem item;
    if (!jsonReadItem(json, i, item)) return true;
    walk->fn(item, walk->ctx);
    jsonSkipWs(json, i);
    if (i < (int)json.length() && json[i] == ',') i++;
  }
}

// Calls fn for each object in the top-level array `key`, in order. True once
// the whole array was read (an empty one counts).
inline bool jsonEachObject(const String &json, const char *key, JsonItemFn fn, void *ctx) {
  JsonItemWalk walk;
  walk.key = key;
  walk.fn = fn;
  walk.ctx = ctx;
  jsonForEachTopField(json, jsonTakeItems, &walk);
  return walk.saw;
}

struct JsonTopItem {
  const char *key;
  JsonItem item;
  bool found = false;
};

inline bool jsonTakeItem(const String &key, const String &json, int &i, void *ctx) {
  auto *hit = static_cast<JsonTopItem *>(ctx);
  if (key != hit->key) return false;
  int at = i;
  hit->found = jsonReadItem(json, i, hit->item);
  if (!hit->found) i = at;
  return true;
}

// The top-level object `key`, read as a flat item.
inline bool jsonTopObject(const String &json, const char *key, JsonItem &out) {
  JsonTopItem hit;
  hit.key = key;
  jsonForEachTopField(json, jsonTakeItem, &hit);
  if (!hit.found) return false;
  out = hit.item;
  return true;
}
