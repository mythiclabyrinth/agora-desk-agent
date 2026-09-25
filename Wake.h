#pragma once

#include <atomic>
#include <stddef.h>
#include <stdint.h>

// On-device wake word: the TFLite Micro audio frontend turns 10 ms of audio
// into 40 log-mel features, and a microWakeWord streaming model (WakeModel.h)
// scores them. Settings match ESPHome's micro_wake_word so its published
// models work unchanged.
//
// Two sides, two cores. The mic task (core 0) calls feed() with every block;
// everything else is for loop() (core 1): it arms the detector when a wake
// would be welcome, deafens it around sounds the board makes, and collects
// detections with takeDetection(). The two sides share only atomics.
class WakeWord {
 public:
  // Loads the model and the frontend. False leaves wake unavailable and the
  // rest of the desk untouched.
  bool begin();
  bool available() const { return _ready; }
  const char *phrase() const;  // "Hey Jarvis", from the model header
  const char *name() const;    // "jarvis": what a transcript may start with

  // --- loop() side ---
  void setArmed(bool armed) { _armedWanted.store(armed); }
  // Listening right now: armed, not held off, not in the refractory period.
  bool listening() const { return _listening.load(); }
  // The speaker or buzzer is (about to be) making sound: stay deaf for `ms`.
  void holdOff(unsigned long ms);
  // 0-255, the unit the detector compares in; ConfigStore clamps it.
  void setCutoff(uint8_t cutoff) { _cutoff.store(cutoff); }
  // The model's own cutoff (WakeModel.h) in the same unit.
  static uint8_t modelCutoff();
  // A detection since the last call? `samplePos` is the mic position at the
  // end of the phrase, so a recording can start exactly there.
  bool takeDetection(uint32_t &samplePos);
  // Smoothed probability 0-1 of the phrase, for tuning in the page.
  float score() const { return _score.load() / 255.0f; }
  // Highest single-step probability in the last WAKE_PEAK_HOLD_MS.
  float peak() const { return _peak.load() / 255.0f; }
  float cutoff() const { return _cutoff.load() / 255.0f; }

  // --- mic task side ---
  // One block of 16 kHz mono; `endPos` is the mic's sample count after it.
  void feed(const int16_t *samples, size_t count, uint32_t endPos);

 private:
  bool loadModel();
  bool featureReady(const uint16_t *values, size_t size, uint32_t now);
  void resetWindow();

  bool _ready = false;
  std::atomic<bool> _armedWanted{false};
  std::atomic<bool> _listening{false};
  std::atomic<uint32_t> _holdUntil{0};
  std::atomic<uint8_t> _cutoff{0};  // probability cutoff, 0-255
  std::atomic<uint8_t> _score{0};
  std::atomic<uint8_t> _peak{0};
  std::atomic<bool> _detected{false};
  std::atomic<uint32_t> _detectedPos{0};

  // Task-only state.
  uint32_t _refractoryUntil = 0;
  uint32_t _peakAt = 0;
  uint16_t _warmup = 0;  // features to feed before a detection counts
  uint8_t _stride = 1;
  uint8_t _strideStep = 0;
  uint8_t _probs[32] = {0};  // sliding window, WAKE_MODEL_WINDOW used
  uint8_t _probIndex = 0;
};

extern WakeWord wakeWord;
