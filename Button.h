#pragma once

#include <Arduino.h>

// A push button to GND with the internal pull-up, debounced by time. Shared
// by the talk button (VoiceFlow) and the dial's knob switch (Dial).
class DebouncedButton {
 public:
  void begin(uint8_t pin);
  // +1 on a settled press, -1 on a settled release, 0 otherwise.
  int8_t poll();
  // How long the button has been down, from the press's first edge; 0 while released.
  unsigned long heldMs() const { return _pressed ? millis() - _pressedAt : 0; }

 private:
  uint8_t _pin = 0;
  bool _pressed = false;
  bool _raw = false;
  unsigned long _changedAt = 0;
  unsigned long _pressedAt = 0;
};

// Tells one click from two, from settled edges. A single is only known once
// the window after its release passes, so poll() reports it late; a second
// press inside the window is a double at once, and its release is swallowed.
class ClickCounter {
 public:
  enum class Click : uint8_t { None, Single, Double };

  explicit ClickCounter(unsigned long windowMs) : _windowMs(windowMs) {}
  Click press(unsigned long now) {
    if (_state == State::Waiting && now - _releasedAt < _windowMs) {
      _state = State::Swallow;
      return Click::Double;
    }
    _state = State::Down;
    return Click::None;
  }
  // True when this release belonged to a click being counted.
  bool release(unsigned long now) {
    if (_state == State::Down) {
      _state = State::Waiting;
      _releasedAt = now;
      return true;
    }
    if (_state == State::Swallow) {
      _state = State::Idle;
      return true;
    }
    return false;
  }
  Click poll(unsigned long now) {
    if (_state != State::Waiting || now - _releasedAt < _windowMs) return Click::None;
    _state = State::Idle;
    return Click::Single;
  }
  void reset() { _state = State::Idle; }

 private:
  enum class State : uint8_t { Idle, Down, Waiting, Swallow };
  unsigned long _windowMs;
  unsigned long _releasedAt = 0;
  State _state = State::Idle;
};
