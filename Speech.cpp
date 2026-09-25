#include "Speech.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>

#include "Audio.h"
#include "Board.h"
#include "Json.h"
#include "Wav.h"

namespace {

constexpr char GROQ_BASE[] = "https://api.groq.com/openai/v1";
constexpr char OPENAI_BASE[] = "https://api.openai.com/v1";
constexpr char BOUNDARY[] = "esp32agent7f3a9c";
// Groq Orpheus rejects anything over 200 characters per request.
constexpr size_t TTS_MAX_CHARS = 200;

String baseUrl(const String &provider) {
  return provider == VOICE_OPENAI ? OPENAI_BASE : GROQ_BASE;
}

const char *providerLabel(const String &provider) {
  return provider == VOICE_OPENAI ? "OpenAI" : "Groq";
}

// OpenAI's gpt-4o TTS takes the accent as an instruction; tts-1 ignores it.
String openaiInstructions(const String &model, const String &accent) {
  String m = model;
  m.toLowerCase();
  if (m.indexOf("gpt-4o") < 0 || m.indexOf("tts") < 0) return "";
  if (accent == "british") return "Speak with a clear British English accent.";
  if (accent == "arabic") return "Speak in Arabic using a Saudi dialect.";
  return "Speak with a clear American English accent.";
}
// Long replies get read in full up to here, then trimmed at a sentence.
constexpr size_t TTS_MAX_TOTAL = 1200;
constexpr unsigned long STREAM_TIMEOUT_MS = 15000;
// A browser clip is held in PSRAM until the page fetches it; ~4 MB is about
// 90 s of 24 kHz speech, more than a clipped reply can produce.
constexpr size_t SYNTH_MAX_BYTES = 4u * 1024 * 1024;
constexpr size_t SYNTH_FIRST_ALLOC = 256u * 1024;

// Sink: the desk amplifier, with a head start. The first
// SPEAKER_PREBUFFER_MS of each piece collect here before the amp starts, so
// a pause in the stream drains this instead of the 60 ms DMA.
struct AmpSink {
  uint8_t *buf = nullptr;
  size_t cap = 0;
  size_t len = 0;
  bool started = false;
};
void ampFlush(AmpSink *a) {
  if (a->len) audio.play(a->buf, a->len);
  a->len = 0;
  a->started = true;
}
bool ampFormat(uint32_t rate, uint16_t channels, uint16_t bits, void *ctx) {
  auto *a = static_cast<AmpSink *>(ctx);
  ampFlush(a);  // the previous piece's tail plays before the format can change
  if (!audio.startPlayback(rate, channels, bits)) return false;
  size_t want = rate * channels * (bits / 8) * SPEAKER_PREBUFFER_MS / 1000;
  if (want != a->cap) {
    free(a->buf);
    a->buf = static_cast<uint8_t *>(psramFound() ? heap_caps_malloc(want, MALLOC_CAP_SPIRAM) : malloc(want));
    a->cap = a->buf ? want : 0;
  }
  a->started = !a->buf;  // no buffer: stream straight through
  return true;
}
bool ampPcm(const uint8_t *data, size_t len, void *ctx) {
  auto *a = static_cast<AmpSink *>(ctx);
  if (!a->started && a->len + len < a->cap) {
    memcpy(a->buf + a->len, data, len);
    a->len += len;
    return true;
  }
  ampFlush(a);
  audio.play(data, len);
  return true;
}

// Sink: a growing PSRAM buffer with room left for the header.
struct WavBuild {
  uint8_t *data = nullptr;
  size_t len = WAV_HEADER_BYTES;
  size_t cap = 0;
  uint32_t rate = 0;
  uint16_t channels = 0;
  uint16_t bits = 0;
};
bool bufFormat(uint32_t rate, uint16_t channels, uint16_t bits, void *ctx) {
  auto *b = static_cast<WavBuild *>(ctx);
  if (b->rate && (b->rate != rate || b->channels != channels || b->bits != bits)) return false;
  b->rate = rate;
  b->channels = channels;
  b->bits = bits;
  return true;
}
bool bufPcm(const uint8_t *data, size_t len, void *ctx) {
  auto *b = static_cast<WavBuild *>(ctx);
  if (b->len + len > b->cap) {
    size_t next = b->cap ? b->cap * 2 : SYNTH_FIRST_ALLOC;
    while (next < b->len + len) next *= 2;
    if (next > SYNTH_MAX_BYTES) return false;
    uint8_t *grown = static_cast<uint8_t *>(
        psramFound() ? heap_caps_realloc(b->data, next, MALLOC_CAP_SPIRAM) : realloc(b->data, next));
    if (!grown) return false;
    b->data = grown;
    b->cap = next;
  }
  memcpy(b->data + b->len, data, len);
  b->len += len;
  return true;
}

uint32_t le32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

uint16_t le16(const uint8_t *p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

// HTTPClient::getStreamPtr() is the raw socket. Both providers stream speech
// with Transfer-Encoding: chunked, so the bytes start with a hex size line,
// not "RIFF". This reader strips that framing (or passes plain bodies through).
class BodyReader {
 public:
  BodyReader(WiFiClient *stream, bool chunked) : _stream(stream), _chunked(chunked) {}

  // Up to n bytes; 0 while waiting, -1 once the body is finished or the socket dropped.
  int read(uint8_t *out, size_t n) {
    if (_done) return -1;
    if (_chunked && _inChunk == 0 && !nextChunk()) return _done ? -1 : 0;
    int available = _stream->available();
    if (available <= 0) {
      if (!_stream->connected() && _stream->available() <= 0) {
        _done = true;
        return -1;
      }
      return 0;
    }
    size_t take = min(static_cast<size_t>(available), n);
    if (_chunked) take = min(take, _inChunk);
    int got = _stream->read(out, take);
    if (got > 0 && _chunked) _inChunk -= got;
    return got;
  }

  // Exactly n bytes, or false when the body ends or stalls first.
  bool readExact(uint8_t *out, size_t n) {
    size_t got = 0;
    unsigned long last = millis();
    while (got < n) {
      int r = read(out + got, n - got);
      if (r < 0) return false;
      if (r > 0) {
        got += r;
        last = millis();
        continue;
      }
      if (millis() - last > STREAM_TIMEOUT_MS) return false;
      delay(1);
    }
    return true;
  }

  bool skip(size_t n) {
    uint8_t bin[256];
    while (n) {
      size_t take = min(n, sizeof(bin));
      if (!readExact(bin, take)) return false;
      n -= take;
    }
    return true;
  }

 private:
  // Consume "<hex-size>[;ext]\r\n" (preceded by the previous chunk's CRLF).
  // Returns true with _inChunk set, or false when still waiting on bytes.
  bool nextChunk() {
    unsigned long last = millis();
    String line;
    while (true) {
      if (_stream->available() > 0) {
        int c = _stream->read();
        if (c < 0) continue;
        last = millis();
        if (c == '\n') {
          line.trim();
          if (!line.length()) continue;  // CRLF that closed the previous chunk
          _inChunk = strtoul(line.c_str(), nullptr, 16);
          if (_inChunk == 0) _done = true;  // terminal chunk; trailers are irrelevant
          return _inChunk > 0;
        }
        if (c != '\r' && line.length() < 32) line += static_cast<char>(c);
        continue;
      }
      if (!_stream->connected() && _stream->available() <= 0) {
        _done = true;
        return false;
      }
      if (millis() - last > STREAM_TIMEOUT_MS) {
        _done = true;
        return false;
      }
      delay(1);
    }
  }

  WiFiClient *_stream;
  bool _chunked;
  size_t _inChunk = 0;
  bool _done = false;
};

// What the board saw when it expected a WAV header, kept printable for the UI.
String peek(const uint8_t *bytes, size_t n) {
  String out;
  for (size_t i = 0; i < n; i++) {
    char c = static_cast<char>(bytes[i]);
    if (c >= 32 && c < 127) out += c;
    else {
      char hex[6];
      snprintf(hex, sizeof(hex), "\\x%02X", bytes[i]);
      out += hex;
    }
  }
  return out;
}

String clipForSpeech(const String &text) {
  String t = text;
  t.trim();
  if (t.length() <= TTS_MAX_TOTAL) return t;
  String head = t.substring(0, TTS_MAX_TOTAL);
  int cut = max(head.lastIndexOf(". "), head.lastIndexOf('\n'));
  if (cut > (int)TTS_MAX_TOTAL / 2) head = head.substring(0, cut + 1);
  head.trim();
  return head;
}

// Back up to a UTF-8 character start so a cut never splits a multibyte glyph.
size_t utf8Floor(const String &s, size_t at) {
  while (at > 0 && at < s.length() && (static_cast<unsigned char>(s[at]) & 0xC0) == 0x80) at--;
  return at;
}

}  // namespace

Speech speech;

void speechChunks(const String &text, void (*fn)(const String &chunk, void *ctx), void *ctx) {
  String t = text;
  t.trim();
  size_t start = 0;
  while (start < t.length()) {
    size_t remaining = t.length() - start;
    size_t take = min(remaining, TTS_MAX_CHARS);
    size_t cut = take;
    if (take < remaining) {
      String window = t.substring(start, utf8Floor(t, start + take));
      int at = window.lastIndexOf(". ");
      at = max(at, window.lastIndexOf("? "));
      at = max(at, window.lastIndexOf("! "));
      at = max(at, window.lastIndexOf('\n'));
      at = max(at, window.lastIndexOf(' '));
      cut = at > 0 ? static_cast<size_t>(at + 1) : window.length();
      if (!cut) cut = take;
    }
    String piece = t.substring(start, start + cut);
    piece.trim();
    if (piece.length()) fn(piece, ctx);
    start += cut;
  }
}

String Speech::apiError(const char *what, const String &provider, int status, const String &body) {
  String message;
  int at = body.indexOf("\"message\"");
  if (at >= 0) {
    int i = body.indexOf('"', at + 9);
    if (i >= 0) jsonParseString(body, i, message);
  }
  String out = what;
  out += " failed (";
  out += providerLabel(provider);
  out += ' ';
  out += String(status);
  out += ")";
  if (message.length()) {
    out += ": ";
    out += message.substring(0, 160);
  }
  return out;
}

void wavFree(WavClip &clip) {
  if (clip.data) free(clip.data);
  clip.data = nullptr;
  clip.len = 0;
}

bool Speech::transcribe(const VoiceSettings &settings, const uint8_t *clip, size_t len,
                        const String &filename, const String &mime, String &text, String &error) {
  text = "";
  String head;
  head.reserve(320);
  head += "--";
  head += BOUNDARY;
  head += "\r\nContent-Disposition: form-data; name=\"model\"\r\n\r\n";
  head += settings.sttModel();
  head += "\r\n--";
  head += BOUNDARY;
  head += "\r\nContent-Disposition: form-data; name=\"response_format\"\r\n\r\njson\r\n--";
  head += BOUNDARY;
  head += "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"";
  head += filename.length() ? filename : String("clip.wav");
  head += "\"\r\nContent-Type: ";
  head += mime.length() ? mime : String("application/octet-stream");
  head += "\r\n\r\n";
  String tail = "\r\n--";
  tail += BOUNDARY;
  tail += "--\r\n";

  // HTTPClient wants one contiguous body; PSRAM makes the copy cheap.
  size_t total = head.length() + len + tail.length();
  uint8_t *body = static_cast<uint8_t *>(
      psramFound() ? heap_caps_malloc(total, MALLOC_CAP_SPIRAM) : malloc(total));
  if (!body) {
    error = "Not enough memory to upload the recording.";
    return false;
  }
  memcpy(body, head.c_str(), head.length());
  memcpy(body + head.length(), clip, len);
  memcpy(body + head.length() + len, tail.c_str(), tail.length());

  WiFiClientSecure tls;
  tls.setInsecure();
  HTTPClient http;
  http.setTimeout(60000);
  http.setConnectTimeout(10000);
  bool ok = false;
  const String &provider = settings.sttProvider;
  if (!http.begin(tls, baseUrl(provider) + "/audio/transcriptions")) {
    error = String("Could not reach ") + providerLabel(provider) + ".";
  } else {
    http.addHeader("Authorization", "Bearer " + settings.sttKey());
    http.addHeader("Content-Type", String("multipart/form-data; boundary=") + BOUNDARY);
    int status = http.POST(body, total);
    String response = status > 0 ? http.getString() : String();
    if (status <= 0) {
      error = "Transcription failed: ";
      error += HTTPClient::errorToString(status);
    } else if (status != 200) {
      error = apiError("Transcription", provider, status, response);
    } else {
      jsonTopString(response, "text", text);
      text.trim();
      ok = true;
    }
    http.end();
  }
  free(body);
  return ok;
}

bool Speech::speak(const VoiceSettings &settings, const String &text, String &error) {
  if (!audio.ampReady()) {
    error = "Speaker is not connected.";
    return false;
  }
  AmpSink amp;
  SpeechSink sink{ampFormat, ampPcm, &amp};
  bool ok = speakTo(settings, text, sink, error);
  ampFlush(&amp);
  free(amp.buf);
  audio.stopPlayback();
  return ok;
}

bool Speech::synthesize(const VoiceSettings &settings, const String &text, WavClip &out, String &error) {
  WavBuild build;
  SpeechSink sink{bufFormat, bufPcm, &build};
  bool ok = speakTo(settings, text, sink, error);
  if (ok && (!build.data || build.len <= WAV_HEADER_BYTES)) {
    ok = false;
    error = "The speech service returned no audio.";
  }
  if (!ok) {
    if (build.data) free(build.data);
    if (!error.length()) error = "The reply is too long to speak here.";
    return false;
  }
  wavWriteHeader(build.data, build.len - WAV_HEADER_BYTES, build.rate, build.channels, build.bits);
  out.data = build.data;
  out.len = build.len;
  return true;
}

bool Speech::speakTo(const VoiceSettings &settings, const String &text, const SpeechSink &sink,
                     String &error) {
  String input = clipForSpeech(text);
  if (!input.length()) {
    error = "Nothing to say.";
    return false;
  }
  const String &provider = settings.ttsProvider;
  const String &key = settings.ttsKey();
  String model = settings.ttsModel();
  String voice = settings.ttsVoice();
  if (provider == VOICE_OPENAI) {
    return speakOnce(provider, key, model, voice, input, openaiInstructions(model, settings.accent), sink,
                     error);
  }
  struct Ctx {
    Speech *self;
    const String *provider, *key, *model, *voice;
    const SpeechSink *sink;
    String *error;
    bool ok;
  } ctx{this, &provider, &key, &model, &voice, &sink, &error, true};
  speechChunks(input, [](const String &chunk, void *raw) {
    auto *c = static_cast<Ctx *>(raw);
    if (!c->ok) return;
    c->ok = c->self->speakOnce(*c->provider, *c->key, *c->model, *c->voice, chunk, "", *c->sink, *c->error);
  }, &ctx);
  return ctx.ok;
}

bool Speech::testKey(const String &provider, const String &key, String &error) {
  WiFiClientSecure tls;
  tls.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  http.setConnectTimeout(10000);
  if (!http.begin(tls, baseUrl(provider) + "/models")) {
    error = String("Could not reach ") + providerLabel(provider) + ".";
    return false;
  }
  http.addHeader("Authorization", "Bearer " + key);
  int status = http.GET();
  bool ok = status == 200;
  if (status <= 0) {
    error = String("Could not reach ") + providerLabel(provider) + ": " + HTTPClient::errorToString(status);
  } else if (!ok) {
    error = apiError("Key check", provider, status, http.getString());
  }
  http.end();
  return ok;
}

bool Speech::speakOnce(const String &provider, const String &key, const String &model, const String &voice,
                       const String &input, const String &instructions, const SpeechSink &sink,
                       String &error) {
  String payload = "{\"model\":\"";
  payload += jsonEscape(model);
  payload += "\",\"voice\":\"";
  payload += jsonEscape(voice);
  payload += "\",\"input\":\"";
  payload += jsonEscape(input);
  payload += "\",\"response_format\":\"wav\"";
  if (instructions.length()) {
    payload += ",\"instructions\":\"";
    payload += jsonEscape(instructions);
    payload += '"';
  }
  payload += '}';

  WiFiClientSecure tls;
  tls.setInsecure();
  HTTPClient http;
  http.setTimeout(60000);
  http.setConnectTimeout(10000);
  if (!http.begin(tls, baseUrl(provider) + "/audio/speech")) {
    error = String("Could not reach ") + providerLabel(provider) + ".";
    return false;
  }
  http.addHeader("Authorization", "Bearer " + key);
  http.addHeader("Content-Type", "application/json");
  const char *wanted[] = {"Transfer-Encoding", "Content-Type"};
  http.collectHeaders(wanted, 2);
  int status = http.POST(payload);
  if (status <= 0) {
    error = "Speech failed: ";
    error += HTTPClient::errorToString(status);
    http.end();
    return false;
  }
  if (status != 200) {
    error = apiError("Speech", provider, status, http.getString());
    http.end();
    return false;
  }

  // Both providers stream WAV; Groq uses 0xFFFFFFFF sizes and may slip a LIST
  // chunk in before the data, so walk the chunks instead of trusting 44 bytes.
  String te = http.header("Transfer-Encoding");
  te.toLowerCase();
  BodyReader body(http.getStreamPtr(), te.indexOf("chunked") >= 0);
  uint8_t hdr[12];
  bool ok = body.readExact(hdr, sizeof(hdr));
  if (!ok) {
    error = "Speech response ended before any audio arrived.";
  } else if (memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4)) {
    ok = false;
    error = "Speech response was not a WAV file (";
    error += http.header("Content-Type");
    error += ", starts \"";
    error += peek(hdr, sizeof(hdr));
    error += "\").";
  }
  bool haveFmt = false;
  uint16_t format = 0;
  uint32_t rate = 0;
  uint16_t channels = 0;
  uint16_t bits = 0;
  while (ok) {
    uint8_t ch[8];
    if (!body.readExact(ch, sizeof(ch))) {
      ok = false;
      error = "Speech stream ended early.";
      break;
    }
    uint32_t size = le32(ch + 4);
    if (!memcmp(ch, "fmt ", 4)) {
      if (size < 16 || size > 64) {
        ok = false;
        error = "Speech WAV format is unreadable.";
        break;
      }
      uint8_t fmt[64];
      if (!body.readExact(fmt, size)) {
        ok = false;
        error = "Speech stream ended early.";
        break;
      }
      format = le16(fmt);
      channels = le16(fmt + 2);
      rate = le32(fmt + 4);
      bits = le16(fmt + 14);
      haveFmt = true;
      if (size & 1) body.skip(1);
    } else if (!memcmp(ch, "data", 4)) {
      Serial.printf("Speech: WAV format %u, %lu Hz, %u ch, %u-bit, data %s\n", format, (unsigned long)rate, channels,
                    bits, size == 0xFFFFFFFFu ? "streamed" : String(size).c_str());
      // 1 is PCM; 0xFFFE (extensible) carries 16-bit PCM from every speech API.
      bool pcmWav = format == 1 || format == 0xFFFE;
      if (!haveFmt || !pcmWav || bits != 16 || !sink.format(rate, channels, bits, sink.ctx)) {
        ok = false;
        error = "Cannot play this audio format.";
        break;
      }
      bool untilClose = size == 0xFFFFFFFFu;
      size_t remaining = size;
      // The socket hands over arbitrary byte counts, and the I2S DMA takes
      // bytes verbatim, so a read that ends mid-sample would shift every
      // sample after it by a byte. Only whole frames reach the sink; the
      // tail of a split frame waits for the next read.
      const size_t frame = channels * (bits / 8);
      uint8_t pcm[1024];
      size_t held = 0;
      size_t total = 0;
      size_t splitReads = 0;
      bool logged = false;
      unsigned long last = millis();
      while (untilClose || remaining) {
        size_t want = sizeof(pcm) - held;
        if (!untilClose) want = min(want, remaining);
        int got = body.read(pcm + held, want);
        if (got < 0) break;  // body finished: normal end for an open-ended data chunk
        if (got == 0) {
          if (millis() - last > STREAM_TIMEOUT_MS) {
            ok = false;
            error = "Speech stream stalled.";
            break;
          }
          delay(1);
          continue;
        }
        last = millis();
        if (!untilClose) remaining -= got;
        size_t have = held + got;
        size_t whole = have - have % frame;
        if (have != whole) splitReads++;
        if (!logged && whole >= 16) {
          logged = true;
          Serial.print("Speech: first samples");
          for (size_t i = 0; i < 16; i += 2) Serial.printf(" %d", static_cast<int16_t>(le16(pcm + i)));
          Serial.println();
        }
        if (whole && !sink.pcm(pcm, whole, sink.ctx)) {
          ok = false;
          error = "The reply is too long to speak here.";
          break;
        }
        total += whole;
        held = have - whole;
        if (held) memmove(pcm, pcm + whole, held);
      }
      Serial.printf("Speech: %u PCM bytes, %u reads ended mid-sample\n", (unsigned)total, (unsigned)splitReads);
      break;
    } else {
      if (size == 0xFFFFFFFFu || !body.skip(size + (size & 1))) {
        ok = false;
        error = "Speech WAV is malformed.";
        break;
      }
    }
  }
  http.end();
  return ok;
}
