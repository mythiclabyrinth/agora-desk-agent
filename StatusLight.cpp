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

// The steady colour for the current state, once no flash is showing.
void StatusLight::resting() {
  _lastLevel = 255;
  if (_voice == VoicePhase::Recording) show(RGB_STATUS_LEVEL, RGB_STATUS_LEVEL * 2 / 5, 0);
  else if (_voice == VoicePhase::Transcribing) show(RGB_STATUS_LEVEL / 3, RGB_STATUS_LEVEL * 2 / 15, 0);
  else if (_chat != Phase::Listening) show(0, 0, 0);
  // Listening breathes from update().
}

void StatusLight::follow(Phase chat, VoicePhase voice) {
  bool chatChanged = chat != _chat;
  bool voiceChanged = voice != _voice;
  if (!chatChanged && !voiceChanged) return;
  Phase previous = _chat;
  _chat = chat;
  _voice = voice;
  if (chatChanged && chat == Phase::Listening) {
    flash(RGB_STATUS_LEVEL, RGB_STATUS_LEVEL, RGB_STATUS_LEVEL, RGB_STATUS_FLASH_MS);
    _breatheFrom = millis();
    return;
  }
  if (chatChanged && previous == Phase::Listening && chat == Phase::Done) {
    flash(0, RGB_STATUS_LEVEL, 0, RGB_STATUS_DONE_MS);
    return;
  }
  if (chatChanged && previous == Phase::Listening && chat == Phase::Failed) {
    flash(RGB_STATUS_LEVEL, 0, 0, RGB_STATUS_DONE_MS);
    return;
  }
  if (!_flashUntil) resting();
}

void StatusLight::update() {
  unsigned long now = millis();
  if (_flashUntil) {
    if (static_cast<long>(now - _flashUntil) < 0) return;
    _flashUntil = 0;
    resting();
  }
  if (_chat != Phase::Listening || _voice == VoicePhase::Recording || _voice == VoicePhase::Transcribing) return;
  // A triangle wave between a dim floor and the full level, one cycle per
  // RGB_STATUS_BREATHE_MS; the LED is only rewritten when the level moves.
  unsigned long t = (now - _breatheFrom) % RGB_STATUS_BREATHE_MS;
  unsigned long half = RGB_STATUS_BREATHE_MS / 2;
  unsigned long rise = t < half ? t : RGB_STATUS_BREATHE_MS - t;
  uint8_t level = RGB_STATUS_FLOOR + (RGB_STATUS_LEVEL - RGB_STATUS_FLOOR) * rise / half;
  if (level == _lastLevel) return;
  _lastLevel = level;
  show(level * 3 / 5, 0, level);
}
