#pragma once

#include <Arduino.h>
#include <ESP_I2S.h>

// Recordings and the speaker. A recording is one WAV buffer in PSRAM filled
// from the Mic task's ring; the MAX98357A plays PCM handed to it a block at a
// time. The I2S mic itself belongs to Mic.
class Audio {
 public:
  void begin();
  bool micReady() const;
  bool ampReady() const { return _ampOk; }

  // Recording: start() reserves the buffer and places the read cursor
  // `preRollMs` in the past, pump() copies what the mic task has captured
  // since (call it from loop while recording), stop() writes the WAV header.
  bool startRecording(unsigned long preRollMs);
  void pumpRecording();
  void stopRecording();
  void discardRecording();
  bool recordingFull() const { return _len >= _cap; }
  unsigned long recordedMs() const;
  const uint8_t *wav() const { return _buf; }
  size_t wavSize() const { return _len; }

  // Playback: configure the amp for the WAV format, then feed PCM.
  bool startPlayback(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample);
  size_t play(const uint8_t *pcm, size_t len);
  void stopPlayback();
  bool playing() const { return _playing; }

 private:
  void writeWavHeader();

  I2SClass _amp{I2S_NUM_1};
  bool _ampOk = false;

  uint8_t *_buf = nullptr;
  size_t _cap = 0;   // total bytes including the 44-byte header
  size_t _len = 0;   // bytes used so far, header included
  unsigned long _recordStarted = 0;
  uint32_t _cursor = 0;  // next mic sample to copy

  uint32_t _playRate = 0;
  uint16_t _playChannels = 0;
  bool _playing = false;
};

extern Audio audio;
