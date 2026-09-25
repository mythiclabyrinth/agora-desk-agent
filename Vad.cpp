#include "Vad.h"

#include <math.h>

#include "Board.h"

namespace {

constexpr uint16_t HANGOVER_FRAMES = VAD_HANGOVER_MS / MIC_BLOCK_MS;
constexpr float FRAME_S = MIC_BLOCK_MS / 1000.0f;

}  // namespace

void Vad::reset() {
  _primed = false;
  _speech = false;
  _loud = false;
  _level = 0;
  _floor = 0;
  _run = 0;
  _hang = 0;
}

bool Vad::process(const int16_t *samples, size_t count) {
  if (!count) return _speech;

  // Remove the frame's DC first; a small offset would otherwise read as a
  // constant hum and lift the floor.
  int32_t sum = 0;
  for (size_t i = 0; i < count; i++) sum += samples[i];
  float mean = static_cast<float>(sum) / count;
  float energy = 0;
  for (size_t i = 0; i < count; i++) {
    float v = samples[i] - mean;
    energy += v * v;
  }
  _level = 10.0f * log10f(energy / count + 1.0f);

  if (!_primed) {
    _primed = true;
    _floor = _level < VAD_FLOOR_MIN_DB ? VAD_FLOOR_MIN_DB : _level;
  }

  _loud = _level > _floor + VAD_MARGIN_DB;
  _run = _loud ? (_run < 255 ? _run + 1 : 255) : 0;
  if (_run >= VAD_ONSET_FRAMES) {
    _speech = true;
    _hang = HANGOVER_FRAMES;
  } else if (_hang) {
    _hang--;
  } else {
    _speech = false;
  }

  // Asymmetric floor: drop quickly toward a quieter room, creep up slowly so
  // a sentence does not become the new "silence". While someone talks it
  // creeps at a quarter of the rate, so a long request still ends cleanly,
  // but a fan switched on mid-sentence is eventually absorbed.
  if (_level < _floor) {
    _floor += (_level - _floor) * VAD_FLOOR_FALL;
  } else {
    float step = VAD_FLOOR_RISE_DB_PER_S * FRAME_S * (_speech ? 0.25f : 1.0f);
    float gap = _level - _floor;
    _floor += gap < step ? gap : step;
  }
  if (_floor < VAD_FLOOR_MIN_DB) _floor = VAD_FLOOR_MIN_DB;
  return _speech;
}

VadVerdict vadVerdict(unsigned long elapsedMs, unsigned long speechMs, unsigned long silenceMs) {
  bool heard = speechMs >= VAD_MIN_SPEECH_MS;
  if (heard) return silenceMs >= VAD_END_SILENCE_MS ? VadVerdict::Done : VadVerdict::Listening;
  // Nobody has said enough yet. Give up at the deadline, unless a word is
  // starting right now.
  bool talkingNow = silenceMs < VAD_HANGOVER_MS;
  if (elapsedMs >= VAD_NO_SPEECH_MS && !talkingNow) return VadVerdict::NoSpeech;
  return VadVerdict::Listening;
}
