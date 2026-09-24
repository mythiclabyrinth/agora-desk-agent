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

class Speech {
 public:
  // Transcribe an audio clip. `filename` tells the API the container
  // (desk.wav, clip.webm, clip.mp4…). Empty text with ok=true means silence.
  bool transcribe(const VoiceSettings &settings, const uint8_t *clip, size_t len,
                  const String &filename, const String &mime, String &text, String &error);

  // Speak text through the desk amplifier.
  bool speak(const VoiceSettings &settings, const String &text, String &error);

  // Render text to a WAV in memory for the browser to play.
  bool synthesize(const VoiceSettings &settings, const String &text, WavClip &out, String &error);

  // Cheap auth probe: list models, nothing billed.
  bool testKey(const String &provider, const String &key, String &error);

 private:
  bool speakTo(const VoiceSettings &settings, const String &text, const SpeechSink &sink, String &error);
  bool speakOnce(const String &provider, const String &key, const String &model, const String &voice,
                 const String &input, const String &instructions, const SpeechSink &sink, String &error);
  static String apiError(const char *what, const String &provider, int status, const String &body);
};

// Split text into Orpheus-sized pieces at sentence-ish boundaries.
void speechChunks(const String &text, void (*fn)(const String &chunk, void *ctx), void *ctx);

extern Speech speech;
