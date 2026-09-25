#include "Mic.h"

#include <esp_heap_caps.h>

#include "Board.h"
#include "Wake.h"

namespace {

constexpr size_t BLOCK_SAMPLES = MIC_SAMPLE_RATE * MIC_BLOCK_MS / 1000;  // 160

}  // namespace

Mic mic;

void Mic::begin() {
  _i2s.setPins(MIC_BCLK_PIN, MIC_WS_PIN, -1, MIC_DATA_PIN);
  // The INMP441 puts 24 bits MSB-first in a 32-bit slot. Ask the driver for
  // 32-bit frames and let it fold them to 16-bit, which keeps the top bits.
  _ok = _i2s.begin(I2S_MODE_STD, MIC_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO,
                   I2S_STD_SLOT_LEFT) &&
        _i2s.configureRX(MIC_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_RX_TRANSFORM_32_TO_16);
  // A block is 10 ms; if nothing arrives in 100 the mic is gone, and the task
  // should not spin.
  _i2s.setTimeout(100);
  if (!_ok) {
    Serial.println("Mic missing");
    return;
  }

  _ringSamples = MIC_RING_SAMPLES;
  if (psramFound()) {
    _ring = static_cast<int16_t *>(heap_caps_malloc(_ringSamples * sizeof(int16_t), MALLOC_CAP_SPIRAM));
  }
  if (!_ring) {
    _ringSamples = MIC_RING_SAMPLES_NO_PSRAM;
    _ring = static_cast<int16_t *>(malloc(_ringSamples * sizeof(int16_t)));
  }
  if (!_ring) {
    Serial.println("Mic: no memory for the audio ring");
    _ok = false;
    return;
  }

  // Core 0 keeps the audio path clear of loop()'s blocking HTTP calls.
  if (xTaskCreatePinnedToCore(taskEntry, "mic", MIC_TASK_STACK, this, MIC_TASK_PRIORITY, nullptr, MIC_TASK_CORE) !=
      pdPASS) {
    Serial.println("Mic: could not start the capture task");
    _ok = false;
    return;
  }
  Serial.print("Mic ok, ");
  Serial.print(_ringSamples * 1000 / MIC_SAMPLE_RATE);
  Serial.println(" ms ring");
}

void Mic::taskEntry(void *self) {
  static_cast<Mic *>(self)->run();
}

void Mic::run() {
  int16_t block[BLOCK_SAMPLES];
  const size_t mask = _ringSamples - 1;
  for (;;) {
    size_t got = _i2s.readBytes(reinterpret_cast<char *>(block), sizeof(block)) / sizeof(int16_t);
    if (!got) {
      // Timed out or errored; readBytes already waited, so this cannot spin.
      continue;
    }

    if (MIC_GAIN > 1) {
      for (size_t i = 0; i < got; i++) {
        int32_t v = static_cast<int32_t>(block[i]) * MIC_GAIN;
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        block[i] = static_cast<int16_t>(v);
      }
    }

    uint32_t pos = _written.load(std::memory_order_relaxed);
    for (size_t i = 0; i < got; i++) _ring[(pos + i) & mask] = block[i];
    pos += got;
    _written.store(pos, std::memory_order_release);

    uint32_t frame = _frames.load(std::memory_order_relaxed) + 1;
    if (_vad.process(block, got)) _speechFrames.fetch_add(1, std::memory_order_relaxed);
    if (_vad.loud()) _lastLoudFrame.store(frame, std::memory_order_relaxed);
    _frames.store(frame, std::memory_order_release);

    wakeWord.feed(block, got, pos);
  }
}

size_t Mic::history() const {
  // Keep one block of slack: the task may be writing the oldest slot now.
  uint32_t written = position();
  size_t keep = _ringSamples - BLOCK_SAMPLES;
  return written < keep ? written : keep;
}

size_t Mic::read(uint32_t &cursor, int16_t *dst, size_t maxSamples, bool *lost) const {
  if (lost) *lost = false;
  if (!_ok) return 0;
  uint32_t written = position();
  uint32_t behind = written - cursor;
  size_t keep = _ringSamples - BLOCK_SAMPLES;
  if (behind > keep) {
    // The ring lapped this reader; skip to the oldest samples still intact.
    cursor = written - keep;
    behind = keep;
    if (lost) *lost = true;
  }
  size_t n = behind < maxSamples ? behind : maxSamples;
  const size_t mask = _ringSamples - 1;
  for (size_t i = 0; i < n; i++) dst[i] = _ring[(cursor + i) & mask];
  cursor += n;
  return n;
}

VadStats Mic::vad() const {
  VadStats s;
  s.frames = _frames.load(std::memory_order_acquire);
  s.speechFrames = _speechFrames.load(std::memory_order_relaxed);
  s.lastLoudFrame = _lastLoudFrame.load(std::memory_order_relaxed);
  return s;
}
