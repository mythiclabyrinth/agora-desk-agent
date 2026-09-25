#pragma once

#include <Arduino.h>
#include <stdio.h>

// Text for a fixed-width HD44780 line. Pure, so a host test covers it.

// Blank columns between the end of a scrolling line and its restart.
constexpr size_t SCROLL_GAP = 4;

// Replies are UTF-8 and often markdown; the LCD's ROM is ASCII plus katakana
// and draws '\' and '~' as a yen sign and an arrow. Typographic punctuation
// becomes ASCII, any other non-ASCII character '?', control characters and
// runs of whitespace one space. Codes 8-15 (glyphs) cannot come through.
inline String lcdText(const String &in, size_t maxLen) {
  String out;
  out.reserve(in.length() < maxLen ? in.length() : maxLen);
  auto put = [&](const char *s) {
    for (; *s && out.length() < maxLen; s++) {
      if (*s == ' ' && (!out.length() || out[out.length() - 1] == ' ')) continue;
      out += *s;
    }
  };
  size_t i = 0;
  while (i < in.length() && out.length() < maxLen) {
    uint8_t c = static_cast<uint8_t>(in[i]);
    if (c < 0x80) {
      i++;
      char one[2] = {static_cast<char>(c), 0};
      if (c < 0x20 || c == 0x7F) one[0] = ' ';
      else if (c == '\\') one[0] = '/';
      else if (c == '~') one[0] = '-';
      else if (c == '*' || c == '`') continue;
      put(one);
      continue;
    }
    size_t n = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
    uint32_t cp = 0;
    if (n == 2 && i + 1 < in.length()) {
      cp = ((c & 0x1F) << 6) | (static_cast<uint8_t>(in[i + 1]) & 0x3F);
    } else if (n == 3 && i + 2 < in.length()) {
      cp = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(in[i + 1]) & 0x3F) << 6) |
           (static_cast<uint8_t>(in[i + 2]) & 0x3F);
    }
    i += n;
    if (cp == 0x2018 || cp == 0x2019) put("'");
    else if (cp == 0x201C || cp == 0x201D) put("\"");
    else if (cp == 0x2013 || cp == 0x2014) put("-");
    else if (cp == 0x2026) put("...");
    else if (cp == 0x00A0) put(" ");
    else if (cp == 0x203A || cp == 0x2192) put(">");
    else put("?");
  }
  while (out.length() && out[out.length() - 1] == ' ') out.remove(out.length() - 1);
  return out;
}

// Exactly `width` columns: cut or padded with spaces.
inline String fitText(const String &s, size_t width) {
  if (s.length() >= width) return s.substring(0, width);
  String out = s;
  while (out.length() < width) out += ' ';
  return out;
}

// `left` at the start, `right` flush with the end, at least one space between.
inline String alignEnds(const String &left, const String &right, size_t width) {
  if (!right.length()) return fitText(left, width);
  if (right.length() >= width) return right.substring(0, width);
  String out = fitText(left, width - right.length() - 1);
  out += ' ';
  out += right;
  return out;
}

// The `width` columns visible after `step` scroll steps. Text that fits is
// padded and never moves; longer text loops with SCROLL_GAP blanks between.
inline void scrollWindow(const char *text, size_t len, unsigned long step, char *out, size_t width) {
  if (len <= width) {
    for (size_t i = 0; i < width; i++) out[i] = i < len ? text[i] : ' ';
    return;
  }
  size_t period = len + SCROLL_GAP;
  size_t start = step % period;
  for (size_t i = 0; i < width; i++) {
    size_t k = (start + i) % period;
    out[i] = k < len ? text[k] : ' ';
  }
}

// Scroll steps shown `elapsedMs` after a line appeared: still for `holdMs`, then one per `stepMs`.
inline unsigned long scrollStep(unsigned long elapsedMs, unsigned long holdMs, unsigned long stepMs) {
  return elapsedMs < holdMs ? 0 : (elapsedMs - holdMs) / stepMs + 1;
}

// "m:ss".
inline String clockText(unsigned long ms) {
  unsigned long s = ms / 1000;
  char buf[16];
  snprintf(buf, sizeof buf, "%lu:%02lu", s / 60, s % 60);
  return String(buf);
}
