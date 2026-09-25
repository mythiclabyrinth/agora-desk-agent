#pragma once

#include <Arduino.h>

#include "Config.h"

// Speech clients, mirroring Agora's voice.rs. Transcription is the same
// multipart call on both providers. OpenAI speaks a whole reply in one WAV;
// Groq Orpheus takes <=200 characters at a time, so it is chunked and each
// chunk streams as it arrives. Endpoints are fixed on purpose; only keys and
// model names come from settings.
//
// Audio can land in two places: the desk speaker (I2S) or a PSRAM buffer that
// the web page fetches and plays itself. Both go through SpeechSink.
struct SpeechSink {
  bool (*format)(uint32_t rate, uint16_t channels, uint16_t bits, void *ctx);
  bool (*pcm)(const uint8_t *data, size_t len, void *ctx);
  void *ctx;
};

// A finite, browser-playable WAV assembled in PSRAM. Free with wavFree().
struct WavClip {
  uint8_t *data = nullptr;
  size_t len = 0;
};
void wavFree(WavClip &clip);

// millis() stamps of the last speak(), 0 where a step never happened.
struct SpeechTiming {
  uint32_t request = 0;    // first TTS request sent
  uint32_t headers = 0;    // its response headers
  uint32_t firstPcm = 0;   // first PCM parsed from the stream
  uint32_t playStart = 0;  // first PCM handed to the amp
  uint32_t playEnd = 0;    // last PCM handed to the amp
};

class Speech {
 public:
  // Transcribe an audio clip. `filename` tells the API the container
  // (desk.wav, clip.webm, clip.mp4…). Empty text with ok=true means silence.
  bool transcribe(const VoiceSettings &settings, const uint8_t *clip, size_t len,
                  const String &filename, const String &mime, String &text, String &error);

  // Speak text through the desk amplifier.
  bool speak(const VoiceSettings &settings, const String &text, String &error);
  const SpeechTiming &timing() const { return _timing; }

  // Render text to a WAV in memory for the browser to play.
  bool synthesize(const VoiceSettings &settings, const String &text, WavClip &out, String &error);

  // Cheap auth probe: list models, nothing billed.
  bool testKey(const String &provider, const String &key, String &error);

 private:
  bool speakTo(const VoiceSettings &settings, const String &text, const SpeechSink &sink, String &error);
  bool speakOnce(const String &provider, const String &key, const String &model, const String &voice,
                 const String &input, const String &instructions, const SpeechSink &sink, String &error);
  static String apiError(const char *what, const String &provider, int status, const String &body);

  SpeechTiming _timing;
  // The piece in flight; the amp sink copies these when its audio starts.
  uint32_t _pieceRequest = 0;
  uint32_t _pieceHeaders = 0;
};

// Split text into Orpheus-sized pieces at sentence-ish boundaries.
void speechChunks(const String &text, void (*fn)(const String &chunk, void *ctx), void *ctx);

extern Speech speech;
