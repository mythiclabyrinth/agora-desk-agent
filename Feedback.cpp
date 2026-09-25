#include "Feedback.h"

#include "Board.h"
#include "Wake.h"

Feedback feedback;

void Feedback::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LISTEN_LED_PIN, OUTPUT);
  digitalWrite(LISTEN_LED_PIN, LOW);
}

void Feedback::beep(int duration) {
  // Keep the wake word deaf through the beep and its echo.
  wakeWord.holdOff(duration + WAKE_QUIET_AFTER_SOUND_MS);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void Feedback::attention() {
  // One short chirp when the message goes out.
  beep(90);
}

void Feedback::success() {
  // Two notes, the second longer: the reply arrived.
  beep(60);
  delay(90);
  beep(220);
}

void Feedback::error() {
  for (int i = 0; i < 3; i++) {
    beep(80);
    delay(80);
  }
}

void Feedback::tick() {
  beep(30);
}

void Feedback::wakeChime() {
  beep(40);
  delay(70);
  beep(40);
}

void Feedback::cancel() {
  // An active buzzer has one pitch; "low" here means long and single.
  beep(300);
}

void Feedback::muteToggled(bool muted) {
  if (muted) {
    beep(60);
  } else {
    beep(40);
    delay(70);
    beep(120);
  }
}

void Feedback::agentCue(uint8_t position) {
  for (uint8_t i = 0; i < position; i++) {
    if (i) delay(DIAL_CUE_GAP_MS);
    beep(DIAL_CUE_BEEP_MS);
  }
}

void Feedback::listenLed(bool on) {
  if (_listenLed == (on ? 1 : 0)) return;
  _listenLed = on ? 1 : 0;
  digitalWrite(LISTEN_LED_PIN, on ? HIGH : LOW);
}

void Feedback::follow(Phase phase) {
  if (phase == _phase) return;
  Phase previous = _phase;
  _phase = phase;
  if (phase == Phase::Listening) attention();
  else if (previous == Phase::Listening && phase == Phase::Done) success();
  else if (previous == Phase::Listening && phase == Phase::Failed) error();
}
