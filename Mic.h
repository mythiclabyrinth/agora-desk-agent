#pragma once

#include <Arduino.h>
#include <ESP_I2S.h>
#include <atomic>

#include "Vad.h"

// Counters the VAD keeps in 10 ms frames. They only ever grow (and wrap
// together), so a reader subtracts two snapshots.
struct VadStats {
  uint32_t frames = 0;         // frames processed so far
  uint32_t speechFrames = 0;   // of those, judged speech (hangover included)
  uint32_t lastLoudFrame = 0;  // the most recent frame above the threshold
};

// The INMP441 on I2S port 0, read by one FreeRTOS task on core 0 and nowhere
// else. Every 10 ms block is amplified, handed to the wake-word detector and
// the VAD, and written into a ring buffer in PSRAM. Recordings (Audio) copy
// out of the ring from loop(), so a pre-roll is always there and a slow loop
// iteration no longer drops samples.
//
// The ring is single-producer/single-consumer: the task writes samples, then
// publishes the running sample count with release ordering; readers load it
// with acquire ordering and copy. No locks on the audio path.
class Mic {
 public:
  void begin();
  bool ready() const { return _ok; }

  // Samples written since boot (wraps; compare with unsigned subtraction).
  uint32_t position() const { return _written.load(std::memory_order_acquire); }
  // Copy up to maxSamples from `cursor` onward and advance it. A cursor that
  // fell further behind than the ring holds jumps forward; `lost` says so.
  size_t read(uint32_t &cursor, int16_t *dst, size_t maxSamples, bool *lost = nullptr) const;
  // How far back the ring still reaches from the current position.
  size_t history() const;

  VadStats vad() const;

 private:
  static void taskEntry(void *self);
  void run();

  I2SClass _i2s{I2S_NUM_0};
  bool _ok = false;
  int16_t *_ring = nullptr;
  size_t _ringSamples = 0;  // power of two
  std::atomic<uint32_t> _written{0};

  Vad _vad;
  std::atomic<uint32_t> _frames{0};
  std::atomic<uint32_t> _speechFrames{0};
  std::atomic<uint32_t> _lastLoudFrame{0};
};

extern Mic mic;
