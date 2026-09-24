#pragma once

#include <Arduino.h>

constexpr size_t WAV_HEADER_BYTES = 44;

// Canonical 44-byte PCM header with finite sizes, the shape browsers and
// Whisper both accept. Groq streams WAV with 0xFFFFFFFF sizes, so anything
// we hand on is rewritten through here.
inline void wavWriteHeader(uint8_t *at, uint32_t pcmBytes, uint32_t sampleRate, uint16_t channels,
                           uint16_t bitsPerSample) {
  auto le32 = [](uint8_t *p, uint32_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
  };
  auto le16 = [](uint8_t *p, uint16_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
  };
  uint16_t frame = channels * bitsPerSample / 8;
  memcpy(at, "RIFF", 4);
  le32(at + 4, 36 + pcmBytes);
  memcpy(at + 8, "WAVE", 4);
  memcpy(at + 12, "fmt ", 4);
  le32(at + 16, 16);
  le16(at + 20, 1);  // PCM
  le16(at + 22, channels);
  le32(at + 24, sampleRate);
  le32(at + 28, sampleRate * frame);
  le16(at + 32, frame);
  le16(at + 34, bitsPerSample);
  memcpy(at + 36, "data", 4);
  le32(at + 40, pcmBytes);
}
