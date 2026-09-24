#include "Feedback.h"

#include "Board.h"

namespace {

constexpr unsigned long BLINK_INTERVAL = 280;

}  // namespace

Feedback feedback;

void Feedback::begin() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  allOff();
}

void Feedback::allOff() {
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void Feedback::beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void Feedback::attention() {
  // One short chirp when the message goes out. The green LED blinks after this.
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  beep(90);
}

void Feedback::success() {
  // Two notes, the second longer: the reply arrived. Not the send chirp.
  digitalWrite(LED_PIN, LOW);
  beep(60);
  delay(90);
  beep(220);
  allOff();
}

void Feedback::error() {
  digitalWrite(LED_PIN, LOW);
  for (int i = 0; i < 3; i++) {
    beep(80);
    delay(80);
  }
  allOff();
}

void Feedback::blink() {
  unsigned long now = millis();
  if (now - _lastBlinkAt < BLINK_INTERVAL) return;
  _lastBlinkAt = now;
  _blinkOn = !_blinkOn;
  digitalWrite(LED_PIN, _blinkOn ? HIGH : LOW);
}

void Feedback::tick() {
  beep(30);
}

void Feedback::follow(Phase phase, bool hold) {
  if (phase != _phase) {
    Phase previous = _phase;
    _phase = phase;
    _blinkOn = false;
    _lastBlinkAt = millis();
    if (phase == Phase::Listening) {
      attention();
      _blinkOn = true;
      _lastBlinkAt = millis();
      digitalWrite(LED_PIN, HIGH);
    } else if (previous == Phase::Listening && phase == Phase::Done) {
      success();
    } else if (previous == Phase::Listening && phase == Phase::Failed) {
      error();
    } else {
      allOff();
    }
  }
  if (hold) {
    digitalWrite(LED_PIN, HIGH);
    _held = true;
    return;
  }
  if (_phase == Phase::Listening) blink();
  else if (_held) digitalWrite(LED_PIN, LOW);
  _held = false;
}
