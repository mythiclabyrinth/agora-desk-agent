#include "Display.h"

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <string.h>

#include "Glyphs.h"
#include "LcdText.h"

namespace {

LiquidCrystal_I2C *lcd = nullptr;

bool answers(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void loadGlyph(uint8_t slot, const uint8_t *bitmap) {
  uint8_t rows[8];
  memcpy(rows, bitmap, sizeof rows);
  lcd->createChar(slot, rows);
}

bool due(unsigned long now, unsigned long at) {
  return static_cast<long>(now - at) >= 0;
}

}  // namespace

Display display;

void Display::begin() {
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN, LCD_I2C_HZ);
  if (answers(LCD_I2C_ADDR)) _addr = LCD_I2C_ADDR;
  else if (answers(LCD_I2C_ADDR_ALT)) _addr = LCD_I2C_ADDR_ALT;
  if (!_addr) {
    Serial.println("LCD: none at 0x27 or 0x3F, running without it");
    Wire.end();
    return;
  }
  Serial.printf("LCD: found at 0x%02X\n", _addr);
  lcd = new LiquidCrystal_I2C(_addr, LCD_COLS, LCD_ROWS);
  _present = true;
  _changedAt = millis();
  xTaskCreatePinnedToCore(taskEntry, "lcd", DISPLAY_TASK_STACK, this, DISPLAY_TASK_PRIORITY, nullptr,
                          DISPLAY_TASK_CORE);
}

bool Display::setLine(Line &line, const String &text, unsigned long now) {
  size_t len = text.length() < DISPLAY_TEXT_MAX ? text.length() : DISPLAY_TEXT_MAX;
  if (len == line.len && !memcmp(line.text, text.c_str(), len)) return false;
  memcpy(line.text, text.c_str(), len);
  line.len = len;
  line.since = now;
  return true;
}

void Display::setMain(const String &top, const String &bottom, bool mayDim) {
  if (!_present) return;
  unsigned long now = millis();
  portENTER_CRITICAL(&_mux);
  bool changed = setLine(_main.rows[0], top, now);
  changed = setLine(_main.rows[1], bottom, now) || changed;
  if (changed) _changedAt = now;
  _mayDim = mayDim;
  portEXIT_CRITICAL(&_mux);
}

void Display::notify(const String &top, const String &bottom, unsigned long ms) {
  if (!_present) return;
  unsigned long now = millis();
  portENTER_CRITICAL(&_mux);
  setLine(_notice.rows[0], top, now);
  setLine(_notice.rows[1], bottom, now);
  _noticeOn = true;
  _noticeUntil = now + ms;
  _changedAt = now;
  portEXIT_CRITICAL(&_mux);
}

void Display::taskEntry(void *self) {
  static_cast<Display *>(self)->run();
}

void Display::run() {
  // init() spends ~1 s in the datasheet's power-up delays; here it keeps them off the boot path.
  lcd->init();
  for (uint8_t slot = 0; slot < GLYPH_SLOT_SPINNER; slot++) loadGlyph(slot, GLYPH_BITMAPS[slot]);
  loadGlyph(GLYPH_SLOT_SPINNER, SPINNER_FRAMES[0]);
  TickType_t wake = xTaskGetTickCount();
  for (;;) {
    render(millis());
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
  }
}

void Display::render(unsigned long now) {
  char rows[LCD_ROWS][LCD_COLS];
  portENTER_CRITICAL(&_mux);
  if (_noticeOn && due(now, _noticeUntil)) _noticeOn = false;
  const Page &page = _noticeOn ? _notice : _main;
  for (uint8_t r = 0; r < LCD_ROWS; r++) {
    const Line &line = page.rows[r];
    unsigned long step = scrollStep(now - line.since, DISPLAY_SCROLL_HOLD_MS, DISPLAY_SCROLL_STEP_MS);
    scrollWindow(line.text, line.len, step, rows[r], LCD_COLS);
  }
  bool lit = _noticeOn || !_mayDim || now - _changedAt < DISPLAY_DIM_MS;
  portEXIT_CRITICAL(&_mux);

  // I2C from here on, outside the lock.
  if (lit != _lit) {
    _lit = lit;
    if (lit) lcd->backlight();
    else lcd->noBacklight();
  }
  if (memchr(rows, GLYPH_SPINNER, sizeof rows) && now - _spinAt >= DISPLAY_SPINNER_MS) {
    _spinAt = now;
    _spinFrame = (_spinFrame + 1) % SPINNER_FRAME_COUNT;
    loadGlyph(GLYPH_SLOT_SPINNER, SPINNER_FRAMES[_spinFrame]);
  }
  for (uint8_t r = 0; r < LCD_ROWS; r++) {
    int cursor = -1;  // column the LCD will write next; after createChar it points into CGRAM
    for (uint8_t c = 0; c < LCD_COLS; c++) {
      if (rows[r][c] == _shown[r][c]) continue;
      if (cursor != c) lcd->setCursor(c, r);
      lcd->write(static_cast<uint8_t>(rows[r][c]));
      _shown[r][c] = rows[r][c];
      cursor = c + 1;
    }
  }
}
