#include "StatusLight.h"

#include "Board.h"

StatusLight statusLight;

void StatusLight::begin() {
  show(0, 0, 0);
}

void StatusLight::show(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(RGB_BUILTIN, r, g, b);
}

void StatusLight::flash(uint8_t r, uint8_t g, uint8_t b, unsigned long ms) {
  show(r, g, b);
  _flashUntil = millis() + ms;
}

void StatusLight::follow(Phase phase) {
  if (phase == _phase) return;
  Phase previous = _phase;
  _phase = phase;
  if (phase == Phase::Listening) {
    flash(RGB_STATUS_LEVEL, RGB_STATUS_LEVEL, RGB_STATUS_LEVEL, RGB_STATUS_FLASH_MS);
    _breatheFrom = millis();
    _lastLevel = 255;
  } else if (previous == Phase::Listening && phase == Phase::Done) {
    flash(0, RGB_STATUS_LEVEL, 0, RGB_STATUS_DONE_MS);
  } else if (previous == Phase::Listening && phase == Phase::Failed) {
    flash(RGB_STATUS_LEVEL, 0, 0, RGB_STATUS_DONE_MS);
  } else {
    show(0, 0, 0);
  }
}

void StatusLight::update() {
  unsigned long now = millis();
  if (_flashUntil) {
    if (static_cast<long>(now - _flashUntil) < 0) return;
    _flashUntil = 0;
    if (_phase != Phase::Listening) {
      show(0, 0, 0);
      return;
    }
  }
  if (_phase != Phase::Listening) return;
  // A triangle wave between a dim floor and the full level, one cycle per
  // RGB_STATUS_BREATHE_MS; the LED is only rewritten when the level moves.
  unsigned long t = (now - _breatheFrom) % RGB_STATUS_BREATHE_MS;
  unsigned long half = RGB_STATUS_BREATHE_MS / 2;
  unsigned long rise = t < half ? t : RGB_STATUS_BREATHE_MS - t;
  uint8_t level = RGB_STATUS_FLOOR + (RGB_STATUS_LEVEL - RGB_STATUS_FLOOR) * rise / half;
  if (level == _lastLevel) return;
  _lastLevel = level;
  show(0, level / 3, level);
}
