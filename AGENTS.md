# AGENTS.md

Working guide for AI agents and contributors in this repo. For what the device
*does*, read [README.md](README.md); this file is about how to change it safely.

## What this is

An Arduino sketch for an ESP32-S3 (N16R8) that acts as a thin desk client for
the Agora CLI bridges (Claude, Cursor, Codex): it hosts a web page, posts
`@mention` messages into an Agora channel, and polls for the agent's reply.
Voice (talk button, wake word, browser mic) goes through Groq or OpenAI.

Two languages, one build:

- **Firmware**: C++ (Arduino, esp32 core 3.x). Every `.h`/`.cpp` in the sketch
  root is compiled.
- **Web page**: `web/` (HTML, CSS, vanilla JS). `web/build.py` inlines and gzips
  it into `Page.h`, which the firmware serves from `PROGMEM`.

## Build, run, test

```bash
# firmware (Arduino IDE 2 or arduino-cli), ESP32S3 Dev Module,
# PSRAM=OPI, Flash=16MB, Partition=16M Flash (3MB APP/9.9MB FATFS)
# library: "LiquidCrystal I2C" (Frank de Brabander), from the Library Manager
arduino-cli compile --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB .

# web page
python3 tools/preview.py      # live preview + mock APIs on http://127.0.0.1:8765
python3 web/build.py          # regenerate Page.h  (REQUIRED after editing web/)
python3 web/build.py --check  # CI-style staleness check

# wake word model (WakeModel.h is generated too)
python3 tools/wake/make_model_header.py [model.tflite]
python3 tools/wake/make_model_header.py --check
```

There is no board-side test harness; see [Testing](#testing).

## The page/firmware contract

**`Page.h` is generated. Never edit it by hand.** Edit `web/` and run
`python3 web/build.py`. Commit the regenerated `Page.h` together with the `web/`
change so a fresh clone compiles without Python.

The page talks to the firmware only through the JSON API in `WebUi.cpp`:

| Route | Purpose |
| --- | --- |
| `GET /api/status` | Wi-Fi, agents summary, voice state (polled every few seconds); `voice.agent` is the current exchange's agent, `voice.target_agent` the hands-free setting (dial), `voice.cancelled` a recording the user dropped; `display.present` whether an LCD answered |
| `GET /api/listen` | state of the current chat job |
| `POST /api/chat` | `{agent, text, speak}` → starts a job |
| `GET/POST /api/agents` | per-agent settings (token is write-only) |
| `POST /api/wifi`, `GET /api/wifi/scan` | join / scan |
| `GET/POST /api/voice`, `POST /api/voice/keys`, `POST /api/voice/test` | voice settings; keys write-only; POST fields are optional and the page sends `agent` only when picked, so it never undoes a dial turn |
| `POST /api/voice/talk` | `{action: start\|stop, agent}` drives the board mic from the page |
| `POST /api/voice/wake` | `{enabled?, cutoff?}` wake word on/off (= the dial's long press) and the detector cutoff, 0.50–0.99; allowed mid-exchange → `{wake}` |
| `POST /api/voice/transcribe` | multipart clip from the browser mic → `{text}` |
| `POST /api/voice/say` | `{text}` → finite WAV for the browser to play |
| `POST /api/voice/tone` | a second of 440 Hz through the desk speaker; checks the amp without a provider |

Adding a field: add it to the firmware handler, the page, **and**
`tools/preview.py`'s mock, or the preview will drift from the device.

## Conventions

- **C++**: one class per concern, a global singleton per module
  (`configStore`, `portal`, `chatClient`, `webUi`, `voiceFlow`, `audio`,
  `speech`, `feedback`, `mic`, `wakeWord`, `dial`, `agentDial`, `display`,
  `statusScreen`), `begin()` in `setup()`, `update()`/`handle()` in `loop()`.
  Comments explain *why*, not what. Pins and limits live in `Board.h`; never
  hardcode a GPIO elsewhere.
- **Errors degrade to text.** Handlers return `sendError(code, message)` with a
  sentence a person can act on; nothing panics or reboots. Voice failures leave
  the text reply intact.
- **Memory**: anything larger than a few KB (recordings, speech buffers,
  uploaded clips) goes in PSRAM via `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`
  with a fallback to heap and a hard cap. Check `ESP.getFreeHeap()` before big
  network calls, as `ChatClient` does.
- **JSON**: `Json.h` is a minimal reader (top-level strings/numbers/bools and the
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
- **PCM reaches the amp in whole frames.** `Speech::speakOnce` carries the
  tail of a split sample to the next socket read; the I2S DMA takes bytes
  verbatim, so an odd-length write would shift every later sample by a byte
  and play as noise. The amp sink also holds `SPEAKER_PREBUFFER_MS` of each
  piece before starting, because the DMA is 60 ms deep and the APIs pause.
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
  is absent and the page offers the desk mic instead. Don't "fix" this in JS —
  it's a browser rule. HTTPS on the board would mean replacing `WebServer` with
  `esp_https_server`.
- **Pins** live in `Board.h`, whose header comment maps every header pin.
  Free: GPIO 5 and GPIO 46. 46 is a strapping pin that must not be pulled high
  while GPIO 0 is low (uploads fail): a bare button to GND is fine, a pulled-up
  part is not. Strapping pin 3 (the dial's knob switch) is ignored at boot on a
  stock S3. 8 (SDA) and 7 (SCL) are the LCD's I2C bus; I2S port 0 is the mic,
  port 1 the amp. The KY-040 is powered from 3V3, never 5V.
- **The knob switch has two meanings.** `AgentDial` fires the long press
  (`DIAL_LONG_PRESS_MS`, → `VoiceFlow::toggleMute`) while the knob is still
  held, and the release that follows is not a short press. A short press
  replays the agent cue, except mid-recording, where it does nothing.
- **The dial ISR is IRAM-only.** `Dial::onEdge` and `QuadratureDecoder::step`
  are `IRAM_ATTR`, the table is `DRAM_ATTR`, pins are read with `gpio_ll`, and
  the counter sits behind a `portMUX`. Keep it that way: no `Serial`,
  `digitalRead`, allocation or flash-resident code in there. Bounce is rejected
  by the state table, not by delays.
- **The hands-free agent is cached** (`ConfigStore::voiceAgent()`): the dial
  sets it per click in RAM (`setVoiceAgent`) and saves after
  `DIAL_SAVE_REST_MS` of rest (`saveVoiceAgent`). `voice().agentKey` and
  `saveVoiceFeatures` go through the same cache, so page and dial share one
  value. `AgentDial` defers beeps and the NVS write while a recording is open.
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
  dial's long press, the page toggle and the blue LED (LED = armed or recording).
  The cutoff is stored in the detector's 0-255 unit and shown as 0-1; every
  write clamps it to `WAKE_CUTOFF_MIN..MAX`.
- **The talk button clicks during a wake or page recording.** `ClickCounter`
  (`Button.h`) sends on a single click once `TALK_DOUBLE_CLICK_MS` passes and
  cancels on a double; neither press starts a button recording. Outside those
  recordings it is push-to-talk.
- **Hands-free clips without speech are never sent** (Whisper turns silence
  into "Thank you."). VAD and wake thresholds live in `Board.h`; they are
  untuned on real hardware.
- **The LCD is presentation only.** `Display` is the only code that touches the
  LCD or `Wire`; `StatusScreen` alone decides what it shows, by polling the
  other modules, which never call the display. The one hook is
  `VoiceFlow::onPhaseChange`, because Transcribing and Speaking block `loop()`
  the moment they start. Layouts are pure functions in `Screens.h`.
- **The LCD draws from its own task** (core 1, beside `loop()`), so the spinner
  and a scrolling error move while `loop()` is blocked. Callers only fill a
  framebuffer under a spinlock; the task sends changed cells only (each costs
  ~1.5 ms of I2C). No LCD at 0x27 or 0x3F: no task, and every call is a no-op.
- **Vendored code**: `src/microfrontend` (TFLM, see its README) links against
  the core's prebuilt `kiss_fft_fixed16`; `WakeModel.h` is generated — edit the
  model, not the header.
- **Flash budget**: the default 1.3 MB app partition is too small; the sketch
  needs the 3 MB scheme. Check the "Sketch uses" line after adding code.

## Testing

- Firmware: compile with the exact FQBN above. For pure logic (`Json.h`,
  `Speech::BodyReader`, `Wav.h`, `slugify`, `speechChunks`, `Vad`,
  `wakePhraseEnd`, `QuadratureDecoder`, `dialPick`, `LcdText.h`,
  `Screens.h`), a throwaway host test with a tiny `String`/`millis` shim compiles under clang in seconds; keep
  such files out of the repo or under a `tests/` folder if they become permanent.
- Page: `python3 tools/preview.py`, then exercise the scenarios listed in
  [tools/README.md](tools/README.md). Playwright (from the sibling `agora`
  repo's `node_modules`) works against the preview with
  `--use-fake-device-for-media-stream` for the mic flow. After `web/build.py`,
  confirm `python3 web/build.py --check` passes and the sketch still compiles.

## Git conventions

- Conventional Commits with a scope: `feat(voice): …`, `fix(web): …`,
  `refactor(web): …`, `docs: …`.
- Stage specific files, never `-A`. `Page.h` and its `web/` source change in
  the same commit.
- **Only commit when asked.** Don't push.
- Secrets never enter the repo; `.gitignore` covers `.env*`, `*.pem`,
  `credentials.json`, `wifi.json`.
