#pragma once

#include <Arduino.h>

#include "Board.h"
#include "Glyphs.h"
#include "LcdText.h"

// What each screen says, as pure functions of the desk's state, so a host
// test covers every layout. Lines are exactly LCD_COLS wide, except an error
// meant to scroll, which is longer.

enum class DeskActivity : uint8_t { None, Recording, Thinking, Waiting, Speaking };

struct DeskView {
  const char *agent = "";  // hands-free agent's title
  bool wifi = false;
  String address;  // station IP, or the setup network's when !wifi
  bool wakeAvailable = false;
  bool wakeEnabled = false;
  bool wakeArmed = false;
  const char *wakePhrase = "";
  const char *sensitivity = "medium";

  DeskActivity activity = DeskActivity::None;
  const char *source = "";         // Recording: btn | wake | page
  unsigned long elapsedMs = 0;     // Recording: clip length; Waiting: time waited
  const char *activityAgent = "";  // Waiting, Speaking: the agent asked
};

struct ScreenLines {
  String top;
  String bottom;
};

inline ScreenLines screenLines(const String &top, const String &bottom) {
  ScreenLines s;
  s.top = top;
  s.bottom = bottom;
  return s;
}

inline String glyphText(char glyph, const char *text) {
  String s;
  s += glyph;
  s += ' ';
  s += text;
  return fitText(s, LCD_COLS);
}

inline const char *sensitivityShort(const char *level) {
  if (!strcmp(level, "low")) return "low";
  if (!strcmp(level, "high")) return "high";
  return "med";
}

inline ScreenLines bootScreen() {
  return screenLines(fitText("Desk agent", LCD_COLS), fitText("starting...", LCD_COLS));
}

// Resting: the hands-free agent and whether the desk is listening for it.
inline ScreenLines restingScreen(const DeskView &v) {
  String icons;
  if (!v.wifi) {
    icons += GLYPH_WIFI;
    icons += 'x';
  }
  char wake = 0;
  if (v.wakeAvailable && !v.wakeEnabled) wake = GLYPH_MUTED;
  else if (v.wakeArmed) wake = GLYPH_MIC;
  if (wake) {
    if (icons.length()) icons += ' ';
    icons += wake;
  }
  String bottom;
  if (!v.wifi || !v.wakeAvailable) bottom = fitText(v.address, LCD_COLS);
  else if (!v.wakeEnabled) bottom = fitText("Muted", LCD_COLS);
  else bottom = alignEnds(v.wakePhrase, sensitivityShort(v.sensitivity), LCD_COLS);
  return screenLines(alignEnds(v.agent, icons, LCD_COLS), bottom);
}

inline ScreenLines mainScreen(const DeskView &v) {
  switch (v.activity) {
    case DeskActivity::Recording:
      return screenLines(glyphText(GLYPH_SPINNER, "Listening..."),
                         alignEnds(clockText(v.elapsedMs), v.source, LCD_COLS));
    case DeskActivity::Thinking:
      return screenLines(glyphText(GLYPH_SPINNER, "Thinking..."), fitText("transcribing", LCD_COLS));
    case DeskActivity::Waiting:
      return screenLines(glyphText(GLYPH_CLOCK, "Waiting..."),
                         alignEnds(v.activityAgent, clockText(v.elapsedMs), LCD_COLS));
    case DeskActivity::Speaking:
      return screenLines(glyphText(GLYPH_SPEAKER, "Speaking"), fitText(v.activityAgent, LCD_COLS));
    default:
      return restingScreen(v);
  }
}

// --- Notices, shown over the main screen for a few seconds ---

// `place` is the agent's 1-based position among the `ready` ones, 0 if it is not set up.
inline ScreenLines agentNotice(const char *title, int place, int ready) {
  if (!ready) return screenLines(fitText("No agents set up", LCD_COLS), fitText("Settings>Agents", LCD_COLS));
  String top = "Agent: ";
  top += title;
  String bottom;
  if (!place) {
    bottom = "not set up";
  } else {
    bottom += String(place);
    bottom += " of ";
    bottom += String(ready);
    bottom += " ready";
  }
  return screenLines(fitText(top, LCD_COLS), fitText(bottom, LCD_COLS));
}

inline ScreenLines muteNotice(bool muted, const char *phrase) {
  if (muted) return screenLines(glyphText(GLYPH_MUTED, "Muted"), fitText("wake word off", LCD_COLS));
  String say = "say ";
  say += phrase;
  return screenLines(glyphText(GLYPH_MIC, "Listening"), fitText(say, LCD_COLS));
}

inline ScreenLines wakeNotice(const char *phrase) {
  return screenLines(glyphText(GLYPH_MIC, phrase), fitText("speak now", LCD_COLS));
}

inline ScreenLines missedNotice() {
  return screenLines(glyphText(GLYPH_ALERT, "Didn't catch"), fitText("that - try again", LCD_COLS));
}

// The reply itself is on the page and the speaker; 16 columns cannot carry it.
inline ScreenLines replyNotice(const char *agent) {
  String top = agent;
  top += " replied";
  return screenLines(glyphText(GLYPH_CHECK, top.c_str()), fitText("", LCD_COLS));
}

// The message scrolls when it is longer than the screen.
inline ScreenLines errorNotice(const String &error) {
  String bottom = lcdText(error, DISPLAY_TEXT_MAX);
  if (bottom.length() < LCD_COLS) bottom = fitText(bottom, LCD_COLS);
  return screenLines(glyphText(GLYPH_ALERT, "Error"), bottom);
}

inline ScreenLines wifiNotice(const String &ip) {
  return screenLines(fitText("Wi-Fi connected", LCD_COLS), fitText(ip, LCD_COLS));
}

// No station link: point the user at the setup network.
inline ScreenLines setupNotice(const char *ssid, const String &apIp) {
  String top = "Join ";
  top += ssid;
  return screenLines(fitText(top, LCD_COLS), fitText(apIp, LCD_COLS));
}
