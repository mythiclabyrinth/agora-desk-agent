# AGENTS.md

Working guide for AI agents and contributors in this repo. For what the device
*does*, read [README.md](README.md); this file is about how to change it safely.

## What this is

An Arduino sketch for an ESP32-S3 (N16R8) that acts as a thin desk client for
the Agora CLI bridges (Claude, Cursor, Codex). It hosts a web page, posts
`@mention` messages into an Agora channel over its REST API, polls for the
agent's reply, and gives LED/buzzer feedback. Optional voice: a push-to-talk
button (or an on-device wake word) with an INMP441 mic and MAX98357A speaker,
transcribed and spoken via Groq or OpenAI; the browser can also use its own
mic/speaker with the board proxying the speech APIs.

Two languages, one build:

- **Firmware**: C++ (Arduino, esp32 core 3.x). Every `.h`/`.cpp` in the sketch
  root is compiled.
- **Web page**: `web/` (HTML, CSS, vanilla JS). `web/build.py` inlines and gzips
  it into `Page.h`, which the firmware serves from `PROGMEM`.

## Build, run, test

```bash
# firmware (Arduino IDE 2 or arduino-cli), ESP32S3 Dev Module,
# PSRAM=OPI, Flash=16MB, Partition=16M Flash (3MB APP/9.9MB FATFS)
arduino-cli compile --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB .

# web page
python3 tools/preview.py      # live preview + mock APIs on http://127.0.0.1:8765
python3 web/build.py          # regenerate Page.h  (REQUIRED after editing web/)
python3 web/build.py --check  # CI-style staleness check

# wake word model (WakeModel.h is generated too)
python3 tools/wake/make_model_header.py [model.tflite]
python3 tools/wake/make_model_header.py --check
```

There is no board-side test harness. Verify firmware changes by compiling and,
where possible, by a quick host-side check of pure logic (see
"Testing" below). Verify page changes in the preview; Playwright (from the
sibling `agora` repo's `node_modules`) works against the preview with
`--use-fake-device-for-media-stream` for the mic flow.

## The page/firmware contract

**`Page.h` is generated. Never edit it by hand.** Edit `web/` and run
`python3 web/build.py`. Commit the regenerated `Page.h` together with the `web/`
change so a fresh clone compiles without Python. The preview prints a warning
when `Page.h` is stale.

The page talks to the firmware only through the JSON API in `WebUi.cpp`:

| Route | Purpose |
| --- | --- |
| `GET /api/status` | Wi-Fi, agents summary, voice state (polled every few seconds) |
| `GET /api/listen` | state of the current chat job |
| `POST /api/chat` | `{agent, text, speak}` → starts a job |
| `GET/POST /api/agents` | per-agent settings (token is write-only) |
| `POST /api/wifi`, `GET /api/wifi/scan` | join / scan |
| `GET/POST /api/voice`, `POST /api/voice/keys`, `POST /api/voice/test` | voice settings; keys write-only |
| `POST /api/voice/talk` | `{action: start\|stop, agent}` drives the board mic from the page |
| `POST /api/voice/wake` | `{enabled?, sensitivity?}` wake word on/off (= the mute button) and low\|medium\|high; allowed mid-exchange → `{wake}` |
| `POST /api/voice/transcribe` | multipart clip from the browser mic → `{text}` |
| `POST /api/voice/say` | `{text}` → finite WAV for the browser to play |

Adding a field: add it to the firmware handler, the page, **and**
`tools/preview.py`'s mock, or the preview will drift from the device.

## Conventions

- **C++**: one class per concern, a global singleton per module
  (`configStore`, `portal`, `chatClient`, `webUi`, `voiceFlow`, `audio`,
  `speech`, `feedback`, `mic`, `wakeWord`), `begin()` in `setup()`, `update()`/`handle()` in
  `loop()`. Comments explain *why*, not what. Pins and limits live in
  `Board.h`; never hardcode a GPIO elsewhere.
- **Errors degrade to text.** Handlers return `sendError(code, message)` with a
  sentence a person can act on; nothing panics or reboots. Voice failures leave
  the text reply intact.
- **Memory**: anything larger than a few KB (recordings, speech buffers,
  uploaded clips) goes in PSRAM via `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`
  with a fallback to heap and a hard cap. Check `ESP.getFreeHeap()` before big
  network calls, as `ChatClient` does.
- **JSON**: `Json.h` is a minimal reader (top-level strings/longs/bools and the
  Agora `messages` array). Responses are built by string concatenation; escape
  every user value with `jsonEscape`.
- **Web JS**: classic scripts concatenated in the order `index.html` lists them,
  sharing one global scope. Top-level `let`/`const` that other files read at
  load time belong in `core.js`. No framework, no bundler, no external
  requests. Format with Prettier (`--print-width 110 --single-quote`).
- **Copy**: user-facing strings are calm, specific, and tell the user what to do
  next ("Add the Groq key under Credentials before the mic can be used.").

## Gotchas / invariants

- **The loop is cooperative and single-threaded.** `speech.transcribe/speak/
  synthesize`, `Portal::join` and `Feedback` beeps block; while they run the web
  server does not answer. The page tolerates this (`voiceBusy`/`pageVoice`
  suppress "board unreachable" errors) and `VoiceFlow` waits `SPEAK_GRACE_MS`
  so the page can fetch a reply before the speaker takes over. Keep that
  tolerance if you add blocking work — or move it to a FreeRTOS task.
- **Speech responses are chunked.** `HTTPClient::getStreamPtr()` is the raw
  socket; `Speech::BodyReader` strips `Transfer-Encoding: chunked`. Groq's WAV
  header carries `0xFFFFFFFF` sizes and may include a `LIST` chunk, so the
  parser walks RIFF chunks instead of trusting 44 bytes. Browser-bound audio
  must be a finite WAV — `Speech::synthesize` rewrites the header (`Wav.h`).
- **Groq Orpheus takes ≤ 200 characters per request**; `speechChunks` splits at
  sentence boundaries. Arabic accent swaps to the Arabic model and voices.
- **Secrets are write-only.** Agora tokens and speech keys are never echoed by
  any endpoint (`token_set` / `keys: {groq: true}` only). `ChatClient` drops the
  token from RAM after each job. The board's HTTP API has no auth and NVS is
  unencrypted — don't add anything that returns a stored secret.
- **`@mention` resolution** is by exact `agent_id` or the slug of the display
  name, matching Agora. `ChatClient::sameAgent` picks the reply the same way;
  if a user's replies aren't caught, the id is wrong, not the polling.
- **Browser microphone needs a secure origin.** Over plain HTTP `getUserMedia`
  is absent; the page explains the Chrome flag. Don't "fix" this in JS — it's a
  browser rule. HTTPS on the board would mean replacing `WebServer` with
  `esp_https_server`.
- **Strapping pins** GPIO 3 and 46 stay unused. I2S port 0 is the mic, port 1
  the amp. Pins: LED 5, buzzer 4, talk button 6, mute button 7, blue listening
  LED 8; free on the usable header: 9, 10, 18.
- **The mic has one reader.** `Mic` runs a FreeRTOS task on core 0 that owns
  I2S0; it feeds `wakeWord`, the VAD and a PSRAM ring. `Audio` recordings copy
  from the ring in `loop()` (with a 300 ms pre-roll). Never call `readBytes` on
  the mic elsewhere, and never call VoiceFlow/network code from the task — the
  task only sets atomics that `loop()` consumes (`wakeWord.takeDetection`).
- **No echo cancellation.** Anything that makes sound must deafen the wake word:
  `Feedback::beep` and `Audio::play` call `wakeWord.holdOff()`. New sound paths
  must do the same, or replies containing the phrase will wake the board.
- **Wake settings are cached** in `ConfigStore` (`wake()`), because `loop()`
  reads them every pass; `wake_enabled` is the single source of truth for the
  mute button, the page toggle and the blue LED (LED = armed or recording).
- **Hands-free clips without speech are never sent** (Whisper turns silence
  into "Thank you."). VAD and wake thresholds live in `Board.h`; they are
  untuned on real hardware.
- **Vendored code**: `src/microfrontend` (TFLM, see its README) links against
  the core's prebuilt `kiss_fft_fixed16`; `WakeModel.h` is generated — edit the
  model, not the header.
- **Flash budget**: the default 1.3 MB app partition is too small; the sketch
  needs the 3 MB scheme. Check the "Sketch uses" line after adding code.

## Testing

- Firmware: compile with the exact FQBN above. For pure logic (`Json.h`,
  `Speech::BodyReader`, `Wav.h`, `slugify`, `speechChunks`, `Vad`,
  `wakePhraseEnd`), a throwaway host
  test with a tiny `String`/`millis` shim compiles under clang in seconds; keep
  such files out of the repo or under a `tests/` folder if they become permanent.
- Page: `python3 tools/preview.py`, then exercise the scenarios listed in
  [tools/README.md](tools/README.md). After `web/build.py`, confirm
  `python3 web/build.py --check` passes and the sketch still compiles.

## Git conventions

- Conventional Commits with a scope: `feat(voice): …`, `fix(web): …`,
  `refactor(web): …`, `docs: …`.
- Stage specific files, never `-A`. `Page.h` and its `web/` source change in
  the same commit.
- **Only commit when asked.** Don't push.
- Secrets never enter the repo; `.gitignore` covers `.env*`, `*.pem`,
  `credentials.json`, `wifi.json`.
