#include "Audio.h"

#include <esp_heap_caps.h>

#include "Board.h"
#include "Mic.h"
#include "Wake.h"
#include "Wav.h"

namespace {

constexpr size_t WAV_HEADER = WAV_HEADER_BYTES;

}  // namespace

Audio audio;

void Audio::begin() {
  _amp.setPins(AMP_BCLK_PIN, AMP_LRC_PIN, AMP_DATA_PIN);
  // Orpheus speaks 24 kHz mono; the rate is re-applied per WAV in startPlayback.
  _ampOk = _amp.begin(I2S_MODE_STD, 24000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
  _playRate = 24000;
  _playChannels = 1;

  Serial.print("Speaker ");
  Serial.println(_ampOk ? "ok" : "missing");
}

bool Audio::micReady() const {
  return mic.ready();
}

bool Audio::startRecording(unsigned long preRollMs) {
  if (!mic.ready()) return false;
  discardRecording();

  size_t pcmMax = MIC_SAMPLE_RATE * 2 * (RECORD_MAX_MS / 1000);
  size_t want = WAV_HEADER + pcmMax;
  if (psramFound()) {
    _buf = static_cast<uint8_t *>(heap_caps_malloc(want, MALLOC_CAP_SPIRAM));
  }
  if (!_buf) {
    // No PSRAM: keep a margin for TLS, and shorten the clip to fit.
    size_t spare = ESP.getMaxAllocHeap();
    if (spare < 120000) return false;
    want = min(want, spare - 90000);
    _buf = static_cast<uint8_t *>(malloc(want));
  }
  if (!_buf) return false;
  _cap = want & ~static_cast<size_t>(1);
  _len = WAV_HEADER;
  _recordStarted = millis();

  // The mic never stops, so the clip can begin slightly before the press.
  size_t back = min(static_cast<size_t>(preRollMs * MIC_SAMPLE_RATE / 1000), mic.history());
  _cursor = mic.position() - back;
  return true;
}

void Audio::pumpRecording() {
  if (!_buf || _len >= _cap) return;
  // Everything captured since the last pump; the ring holds seconds, so one
  // copy per loop iteration keeps up even when loop() stalls on a beep.
  size_t room = (_cap - _len) / sizeof(int16_t);
  bool lost = false;
  size_t got = mic.read(_cursor, reinterpret_cast<int16_t *>(_buf + _len), room, &lost);
  if (lost) Serial.println("Voice: recording fell behind the mic; a moment of audio was skipped");
  _len += got * sizeof(int16_t);
}

void Audio::stopRecording() {
  if (!_buf) return;
  writeWavHeader();
}

void Audio::discardRecording() {
  if (_buf) free(_buf);
  _buf = nullptr;
  _cap = 0;
  _len = 0;
  _recordStarted = 0;
}

unsigned long Audio::recordedMs() const {
  if (!_buf) return 0;
  return (_len - WAV_HEADER) * 1000UL / (MIC_SAMPLE_RATE * 2);
}

void Audio::writeWavHeader() {
  wavWriteHeader(_buf, _len - WAV_HEADER, MIC_SAMPLE_RATE, 1, 16);
}

bool Audio::startPlayback(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample) {
  if (!_ampOk) return false;
  if (bitsPerSample != 16 || (channels != 1 && channels != 2)) return false;
  if (sampleRate != _playRate || channels != _playChannels) {
    i2s_slot_mode_t slots = channels == 2 ? I2S_SLOT_MODE_STEREO : I2S_SLOT_MODE_MONO;
    if (!_amp.configureTX(sampleRate, I2S_DATA_BIT_WIDTH_16BIT, slots)) return false;
    _playRate = sampleRate;
    _playChannels = channels;
  }
  _playing = true;
  return true;
}

size_t Audio::play(const uint8_t *pcm, size_t len) {
  if (!_playing || !len) return 0;
  // The mic hears the speaker: keep the wake word deaf through this audio.
  wakeWord.holdOff(WAKE_QUIET_AFTER_SOUND_MS + len * 1000UL / (_playRate * _playChannels * 2));
  return _amp.write(pcm, len);
}

void Audio::stopPlayback() {
  if (!_playing) return;
  // A little silence flushes the DMA so the last syllable is not clipped.
  uint8_t quiet[512] = {0};
  for (int i = 0; i < 4; i++) _amp.write(quiet, sizeof(quiet));
  _playing = false;
  wakeWord.holdOff(WAKE_QUIET_AFTER_SOUND_MS);
}
