#include "Audio.h"

#include <esp_heap_caps.h>

#include "Board.h"
#include "Wav.h"

namespace {

constexpr size_t WAV_HEADER = WAV_HEADER_BYTES;
constexpr size_t PUMP_BYTES = 2048;  // 64 ms of 16-bit mono at 16 kHz

}  // namespace

Audio audio;

void Audio::begin() {
  _mic.setPins(MIC_BCLK_PIN, MIC_WS_PIN, -1, MIC_DATA_PIN);
  // The INMP441 puts 24 bits MSB-first in a 32-bit slot. Ask the driver for
  // 32-bit frames and let it fold them to 16-bit, which keeps the top bits.
  _micOk = _mic.begin(I2S_MODE_STD, MIC_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT,
                      I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT) &&
           _mic.configureRX(MIC_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO,
                            I2S_RX_TRANSFORM_32_TO_16);
  _mic.setTimeout(200);

  _amp.setPins(AMP_BCLK_PIN, AMP_LRC_PIN, AMP_DATA_PIN);
  // Orpheus speaks 24 kHz mono; the rate is re-applied per WAV in startPlayback.
  _ampOk = _amp.begin(I2S_MODE_STD, 24000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
  _playRate = 24000;
  _playChannels = 1;

  Serial.print("Mic ");
  Serial.print(_micOk ? "ok" : "missing");
  Serial.print(", speaker ");
  Serial.println(_ampOk ? "ok" : "missing");
}

bool Audio::startRecording() {
  if (!_micOk) return false;
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
  _cap = want;
  _len = WAV_HEADER;
  _recordStarted = millis();

  // Drop whatever the DMA collected while idle so the clip starts at the press.
  uint8_t scratch[512];
  for (int i = 0; i < 8 && _mic.available() > 0; i++) _mic.readBytes(reinterpret_cast<char *>(scratch), sizeof(scratch));
  return true;
}

void Audio::pumpRecording() {
  if (!_buf || _len >= _cap) return;
  size_t room = _cap - _len;
  size_t want = min(PUMP_BYTES, room) & ~static_cast<size_t>(1);
  if (!want) return;
  size_t got = _mic.readBytes(reinterpret_cast<char *>(_buf + _len), want);
  if (!got) return;

  if (MIC_GAIN > 1) {
    int16_t *samples = reinterpret_cast<int16_t *>(_buf + _len);
    for (size_t i = 0; i < got / 2; i++) {
      int32_t v = static_cast<int32_t>(samples[i]) * MIC_GAIN;
      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;
      samples[i] = static_cast<int16_t>(v);
    }
  }
  _len += got;
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
  return _amp.write(pcm, len);
}

void Audio::stopPlayback() {
  if (!_playing) return;
  // A little silence flushes the DMA so the last syllable is not clipped.
  uint8_t quiet[512] = {0};
  for (int i = 0; i < 4; i++) _amp.write(quiet, sizeof(quiet));
  _playing = false;
}
