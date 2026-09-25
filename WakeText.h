#pragma once

#include <ctype.h>
#include <stddef.h>
#include <string.h>

// A wake recording starts at the detection, but the tail of the phrase (or all
// of it, if the user repeats it) often makes it into the transcript:
// "Hey Jarvis, what's on my list?" -> "what's on my list?". Pure C so it can
// be host-tested; the caller takes text.substring(wakePhraseEnd(...)).

namespace wake_text {

inline bool wordChar(char c) {
  return isalnum(static_cast<unsigned char>(c)) || c == '\'' || (c & 0x80);
}

// Case-insensitive compare of text[0..len) with a lowercase ASCII word.
inline bool sameWord(const char *text, size_t len, const char *word) {
  if (strlen(word) != len) return false;
  for (size_t i = 0; i < len; i++) {
    if (tolower(static_cast<unsigned char>(text[i])) != word[i]) return false;
  }
  return true;
}

inline bool anyWord(const char *text, size_t len, const char *const *words) {
  for (; *words; words++) {
    if (sameWord(text, len, *words)) return true;
  }
  return false;
}

// Skip spaces and punctuation Whisper puts around a greeting: "Hey, Jarvis." / "- Jarvis!".
inline size_t skipGap(const char *text, size_t at) {
  while (text[at] && !wordChar(text[at])) at++;
  return at;
}

inline size_t wordEnd(const char *text, size_t at) {
  while (text[at] && wordChar(text[at])) at++;
  return at;
}

}  // namespace wake_text

// Index where the message starts once a leading wake phrase is removed, or 0
// if the text does not start with one. `name` is the model's wake name in
// lowercase ("jarvis"); "agora" and "jarvis" are always accepted too, so the
// switch to a Hey Agora model changes nothing here.
inline size_t wakePhraseEnd(const char *text, const char *name) {
  using namespace wake_text;
  static const char *const greetings[] = {"hey", "hi", "hay", "hei", "a", "okay", "ok", nullptr};
  static const char *const aliases[] = {"agora", "jarvis", nullptr};
  if (!text) return 0;

  size_t first = skipGap(text, 0);
  size_t firstEnd = wordEnd(text, first);
  if (firstEnd == first) return 0;
  size_t firstLen = firstEnd - first;

  auto isName = [&](size_t at, size_t len) {
    return (name && *name && sameWord(text + at, len, name)) || anyWord(text + at, len, aliases);
  };

  size_t end = 0;
  if (anyWord(text + first, firstLen, greetings)) {
    size_t second = skipGap(text, firstEnd);
    size_t secondEnd = wordEnd(text, second);
    if (secondEnd > second && isName(second, secondEnd - second)) end = secondEnd;
  } else if (isName(first, firstLen) && strchr(",.!?:;", text[firstEnd]) && text[firstEnd]) {
    // A bare name only counts when it is addressed ("Jarvis, ..."), so a
    // message about Agora itself ("Agora is down") keeps its first word.
    end = firstEnd;
  }
  if (!end) return 0;
  return skipGap(text, end);
}
