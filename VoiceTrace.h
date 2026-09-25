#pragma once

#include <stdint.h>
#include <stdio.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

// Where one voice exchange spent its time: millis() stamps, 0 where a step
// never happened. Durations use unsigned subtraction, so they survive the
// 49-day wrap of millis().
struct VoiceTrace {
  uint32_t trigger = 0;      // wake word heard, button pressed, page mic opened
  uint32_t recordStart = 0;
  uint32_t speechEnd = 0;    // the recording ended: VAD pause, release or click
  uint32_t sttStart = 0;
  uint32_t sttDone = 0;
  uint32_t postStart = 0;
  uint32_t postDone = 0;
  uint32_t replySeen = 0;    // the matching reply is known (socket event or poll)
  uint32_t ttsStart = 0;     // first TTS request
  uint32_t ttsHeaders = 0;
  uint32_t ttsFirstPcm = 0;
  uint32_t playStart = 0;    // first PCM handed to the amp
  uint32_t playEnd = 0;
  // Wake recordings: quiet between the last loud frame and speechEnd.
  uint32_t finalSilenceMs = 0;
  bool hasSilence = false;

  void reset() { *this = VoiceTrace(); }

  // to - from in ms, or -1 when either stamp is missing.
  static long span(uint32_t from, uint32_t to) {
    if (!from || !to) return -1;
    return static_cast<long>(static_cast<uint32_t>(to - from));
  }

  // The one-line summary; "-" marks a step the exchange did not reach.
  int format(char *out, size_t size, uint32_t freeHeap) const {
    char vad[12], stt[12], post[12], wait[12], ttfa[12], headers[12], prebuffer[12], total[12];
    field(vad, hasSilence ? static_cast<long>(finalSilenceMs) : -1);
    field(stt, span(sttStart, sttDone));
    field(post, span(postStart, postDone));
    field(wait, span(postDone, replySeen));
    field(ttfa, span(ttsStart, ttsFirstPcm));
    field(headers, span(ttsStart, ttsHeaders));
    field(prebuffer, span(ttsFirstPcm, playStart));
    field(total, span(speechEnd, playStart));
    return snprintf(out, size,
                    "VOICE LATENCY vad=%s stt=%s post=%s agent_wait=%s tts_ttfa=%s (headers=%s) prebuffer=%s "
                    "total_to_speech=%s heap=%lu",
                    vad, stt, post, wait, ttfa, headers, prebuffer, total, static_cast<unsigned long>(freeHeap));
  }

#ifdef ARDUINO
  void print() const {
    char line[200];
    format(line, sizeof(line), ESP.getFreeHeap());
    Serial.println(line);
  }
#endif

 private:
  static void field(char (&out)[12], long ms) {
    if (ms < 0) snprintf(out, sizeof(out), "-");
    else snprintf(out, sizeof(out), "%ld", ms);
  }
};
