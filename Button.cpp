#include "Button.h"

#include "Board.h"

void DebouncedButton::begin(uint8_t pin) {
  _pin = pin;
  pinMode(pin, INPUT_PULLUP);
  _raw = digitalRead(pin) == LOW;
  _pressed = _raw;
  _changedAt = millis();
  _pressedAt = _changedAt;
}

int8_t DebouncedButton::poll() {
  bool raw = digitalRead(_pin) == LOW;
  unsigned long now = millis();
  if (raw != _raw) {
    _raw = raw;
    _changedAt = now;
    return 0;
  }
  if (raw == _pressed || now - _changedAt < BUTTON_DEBOUNCE_MS) return 0;
  _pressed = raw;
  if (_pressed) _pressedAt = _changedAt;
  return _pressed ? 1 : -1;
}
