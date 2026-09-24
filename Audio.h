#pragma once

#include <Arduino.h>
#include <ESP_I2S.h>

// Mic in, speaker out. The INMP441 records into one WAV buffer in PSRAM;
// the MAX98357A plays PCM handed to it a block at a time.
class Audio {
 public:
  void begin();
  bool micReady() const { return _micOk; }
  bool ampReady() const { return _ampOk; }

  // Recording: start() reserves the buffer, pump() pulls the next slice from
  // the mic (call it from loop while recording), stop() writes the WAV header.
  bool startRecording();
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

 private:
  void writeWavHeader();

  I2SClass _mic{I2S_NUM_0};
  I2SClass _amp{I2S_NUM_1};
  bool _micOk = false;
  bool _ampOk = false;

  uint8_t *_buf = nullptr;
  size_t _cap = 0;   // total bytes including the 44-byte header
  size_t _len = 0;   // bytes used so far, header included
  unsigned long _recordStarted = 0;

  uint32_t _playRate = 0;
  uint16_t _playChannels = 0;
  bool _playing = false;
};

extern Audio audio;
