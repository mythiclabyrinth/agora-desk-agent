#pragma once

#include <Arduino.h>

#include "Board.h"

// The 16x2 LCD, and the only code that touches it or the I2C bus. Pure
// presentation: callers say what to show, a task beside loop() draws it,
// so the spinner and scrolling text keep moving while loop() is blocked in
// speech or HTTP calls. Lines wider than the screen scroll; character codes
// 8-15 are glyphs (Glyphs.h). With no LCD on the bus every call is a no-op.
class Display {
 public:
  void begin();
  bool present() const { return _present; }

  // The screen shown when no notice is up. `mayDim` lets the backlight go
  // off once it has not changed for DISPLAY_DIM_MS.
  void setMain(const String &top, const String &bottom, bool mayDim);
  // Shown over the main screen for `ms`; a newer notice replaces it.
  void notify(const String &top, const String &bottom, unsigned long ms);

 private:
  struct Line {
    char text[DISPLAY_TEXT_MAX];
    size_t len = 0;
    unsigned long since = 0;  // when this text appeared; scrolling counts from here
  };
  struct Page {
    Line rows[LCD_ROWS];
  };

  static void taskEntry(void *self);
  void run();
  void render(unsigned long now);
  static bool setLine(Line &line, const String &text, unsigned long now);

  bool _present = false;
  uint8_t _addr = 0;

  // Shared with the task, under _mux.
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
  Page _main;
  Page _notice;
  bool _noticeOn = false;
  unsigned long _noticeUntil = 0;
  bool _mayDim = false;
  unsigned long _changedAt = 0;

  // Task only: what the LCD holds now.
  char _shown[LCD_ROWS][LCD_COLS] = {};
  bool _lit = false;
  uint8_t _spinFrame = 0;
  unsigned long _spinAt = 0;
};

extern Display display;
