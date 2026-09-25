#pragma once

#include <stdint.h>

// Custom 5x8 characters for the HD44780's eight CGRAM slots. Character codes
// 8-15 mirror slots 0-7, so a glyph can sit inside a C string (code 0 would
// end it); these are the codes to embed.
enum GlyphSlot : uint8_t {
  GLYPH_SLOT_MIC,
  GLYPH_SLOT_MUTED,
  GLYPH_SLOT_SPEAKER,
  GLYPH_SLOT_CLOCK,
  GLYPH_SLOT_CHECK,
  GLYPH_SLOT_ALERT,
  GLYPH_SLOT_WIFI,
  GLYPH_SLOT_SPINNER,  // rewritten every frame, so every cell showing it animates
  GLYPH_SLOT_COUNT
};

constexpr char GLYPH_MIC = 8 + GLYPH_SLOT_MIC;
constexpr char GLYPH_MUTED = 8 + GLYPH_SLOT_MUTED;
constexpr char GLYPH_SPEAKER = 8 + GLYPH_SLOT_SPEAKER;
constexpr char GLYPH_CLOCK = 8 + GLYPH_SLOT_CLOCK;
constexpr char GLYPH_CHECK = 8 + GLYPH_SLOT_CHECK;
constexpr char GLYPH_ALERT = 8 + GLYPH_SLOT_ALERT;
constexpr char GLYPH_WIFI = 8 + GLYPH_SLOT_WIFI;
constexpr char GLYPH_SPINNER = 8 + GLYPH_SLOT_SPINNER;

// Static glyphs, in slot order (the spinner's frames follow).
constexpr uint8_t GLYPH_BITMAPS[GLYPH_SLOT_SPINNER][8] = {
    {0b01110, 0b01110, 0b01110, 0b11111, 0b01110, 0b00100, 0b01110, 0b00000},  // mic
    {0b01111, 0b01100, 0b01100, 0b11011, 0b00110, 0b01100, 0b11110, 0b10000},  // mic, struck
    {0b00001, 0b00011, 0b11111, 0b11111, 0b11111, 0b00011, 0b00001, 0b00000},  // speaker
    {0b11111, 0b10001, 0b01010, 0b00100, 0b01110, 0b11111, 0b11111, 0b00000},  // hourglass
    {0b00000, 0b00001, 0b00011, 0b10110, 0b11100, 0b01000, 0b00000, 0b00000},  // check
    {0b00000, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b00000, 0b00000},  // cross
    {0b00000, 0b01110, 0b10001, 0b00100, 0b01010, 0b00000, 0b00100, 0b00000},  // wifi
};

// A turning bar. The ROM has no backslash (0x5C is a yen sign), hence a glyph.
constexpr uint8_t SPINNER_FRAME_COUNT = 4;
constexpr uint8_t SPINNER_FRAMES[SPINNER_FRAME_COUNT][8] = {
    {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000},
    {0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000, 0b00000},
    {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000, 0b00000},
    {0b10000, 0b01000, 0b01000, 0b00100, 0b00010, 0b00010, 0b00001, 0b00000},
};
