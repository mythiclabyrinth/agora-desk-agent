#pragma once

#include <stddef.h>
#include <stdint.h>

// Energy voice-activity detector for 10 ms frames. It only has to tell "a
// person is talking at the desk" from "the room", so it tracks the room's
// noise floor and calls anything well above it speech. Pure logic: the mic
// task owns one and feeds it every block; host tests feed it synthetic frames.
class Vad {
 public:
  void reset();
  // One frame of 16-bit mono. Returns the speech flag, hangover included.
  bool process(const int16_t *samples, size_t count);
  bool speech() const { return _speech; }
  // This frame alone cleared the threshold (no onset wait, no hangover).
  bool loud() const { return _loud; }
  float levelDb() const { return _level; }
  float floorDb() const { return _floor; }

 private:
  bool _primed = false;
  bool _speech = false;
  bool _loud = false;
  float _level = 0;
  float _floor = 0;
  uint8_t _run = 0;       // consecutive loud frames
  uint16_t _hang = 0;     // frames of hangover left
};

// Where a hands-free (wake word) recording stands.
enum class VadVerdict { Listening, Done, NoSpeech };

// elapsedMs since the chime, speechMs of speech heard since then, silenceMs
// since the last loud frame. Ends after a pause that follows real speech, and
// gives up if nobody starts talking.
VadVerdict vadVerdict(unsigned long elapsedMs, unsigned long speechMs, unsigned long silenceMs);
