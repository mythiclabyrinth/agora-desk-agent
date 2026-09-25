# ESP32 Desk Agent

A small desk device, built on an ESP32-S3 (N16R8), that lets you talk to the
Claude, Cursor, and Codex CLI agents you already run through
[Agora](../../agora). The board hosts a web page on your Wi-Fi; you type (or
speak) a message, it posts an `@mention` into an Agora channel, waits for that
agent's reply, and shows it — with an LED and buzzer telling you what's going
on. With a microphone and speaker attached, a push-to-talk button does the same
thing by voice.

The bridges stay on your computer. The board is a thin client: it never runs an
LLM and never holds a model key; it holds an Agora token per agent and, if you
use voice, a Groq and/or OpenAI key for speech.

## What it does

- **Web UI on the board** — Overview, Conversations, Settings (Wi-Fi, Agents,
  Voice). Served from flash, no CDN or internet needed to load.
- **Wi-Fi setup** — first boot opens the `Esp32-Agent` network (password
  `agent-setup`); open `http://192.168.4.1`, scan and pick your network. After
  that the page is at `http://esp32-agent.local` or the board's IP.
- **Three agents** — each with its own Agora URL, channel id, access token,
  display name and agent id (the `@mention` target).
- **Feedback** — one chirp when a message goes out, green LED blinks while
  waiting, two notes when the reply lands, three short beeps on failure.
- **Voice (optional hardware)** — hold the talk button, speak, let go: the
  INMP441 clip is transcribed (Groq Whisper or OpenAI), sent to the agent, and
  the reply is read aloud through a MAX98357A speaker (Groq Orpheus or OpenAI
  TTS). Provider, model, voice and accent are picked per direction under
  Settings › Voice.
- **Wake word (hands-free)** — say "Hey Jarvis" (a "Hey Agora" model can be
  trained and dropped in; see [tools/wake/README.md](tools/wake/README.md)), wait
  for two beeps, speak, and pause. The model runs on the board (microWakeWord on
  TensorFlow Lite Micro); an energy VAD ends the recording when you stop talking
  and drops it if you said nothing. A **mute button** switches the wake word off
  and on (so does Settings › Voice), and a **blue LED** is lit whenever the
  board's mic is listening: wake word armed, or any recording in progress. The
  wake word is deaf while the board beeps or speaks, so a reply that says the
  phrase cannot wake it.
- **Hands-free agent and the dial** — the talk button and the wake word both
  reach one agent, set under Settings › Voice › Devices › Hands-free agent or
  with a KY-040 rotary dial. Turning the dial steps through the *configured*
  agents in the order Claude, Cursor, Codex (wrapping both ways; clockwise =
  next) and, once the knob rests, beeps the new agent's place: one beep for
  Claude, two for Cursor, three for Codex. A short press on the knob replays
  that. With only one agent configured the dial just ticks; with none it gives
  the three error beeps. A turn mid-recording applies to the next message and
  beeps after the recording ends. The page's picker and the Overview follow the
  dial; the choice is saved to flash a moment after the knob stops.
- **Voice from the browser (no extra hardware)** — the chat's mic button records
  with the browser's microphone and the speaker toggle plays replies through the
  browser. The board proxies both to the speech APIs so the keys never leave it.

## Hardware

ESP32-S3 dev module with 8 MB PSRAM and 16 MB flash. Pins are in `Board.h`.

| Part | Pins |
| --- | --- |
| LED (with resistor) | GPIO 5 → LED → GND |
| Active buzzer | GPIO 4 → buzzer → GND |
| Talk button | GPIO 6 → button → GND (internal pull-up) |
| Mute button | GPIO 7 → button → GND (internal pull-up) |
| Blue listening LED | GPIO 8 → 47–100 Ω → LED → GND |
| KY-040 rotary dial | CLK 9, DT 10, SW 18, **+ → 3V3 (not 5V)**, GND → GND |
| INMP441 microphone | SCK 12, WS 11, SD 13, L/R → GND, VDD 3V3 |
| MAX98357A amplifier | BCLK 16, LRC 15, DIN 17, VIN 5V, 4 Ω speaker on +/− |

GPIO 3 and 46 are strapping pins and stay unused; with the dial on 9, 10 and
18 the usable header (GND, 5V, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7,
6, 5, 4, RST, 3V3) is fully used. Power the KY-040 from 3V3: its on-board
pull-ups go to "+", and the ESP32-S3's GPIOs are not 5 V tolerant. The
firmware enables the internal pull-ups too, since many KY-040 boards leave the
switch's pull-up unpopulated. If clockwise selects the previous agent, set
`DIAL_REVERSE` in `Board.h`; if one click moves two agents or every other
click is ignored, adjust `DIAL_STEPS_PER_DETENT`. A blue LED drops about 3 V, so from a 3.3 V
pin it needs a small resistor (47–100 Ω) or it barely glows. The LEDs, buzzer,
buttons, dial, mic and amp are all optional; the page works without any of them.

## Build and flash

Arduino IDE 2 with the `esp32` core 3.x. Board settings:

- Board: **ESP32S3 Dev Module**
- PSRAM: **OPI PSRAM** (recordings and speech buffers live there)
- Flash Size: **16MB**
- Partition Scheme: **16M Flash (3MB APP/9.9MB FATFS)**

Open `Esp32Agent.ino`, compile, upload. Watch the Serial Monitor at 115200 for
the setup-network address and, once joined, the board's IP.

The web page is compiled into the firmware from `web/`. If you change anything
in `web/`, run `python3 web/build.py` first (see [Editing the page](#editing-the-page)).

## Setting up

1. Run the Agora server and at least one bridge (`bridges/claude-cli`, `cursor-cli`,
   `codex-cli`) on your computer, as usual.
2. In Agora, create (or pick) a channel the agent is in and note its channel id.
3. On the board's page, Settings › Wi-Fi: join your network.
4. Settings › Agents: for each agent, enter the Agora URL (e.g.
   `http://192.168.1.20:4470`), the channel id, an access token, and make sure
   **Agent ID** matches the bridge's `AGENT_ID` (the `@mention` is resolved by
   exact id or by the slug of the agent's display name).
5. Optional — Settings › Voice: paste a Groq (`gsk_…`) or OpenAI (`sk-…`) key,
   choose providers/models, and pick the hands-free agent (the one the talk
   button, the wake word and the dial reach) under Devices.

### Browser microphone and HTTPS

Browsers only expose the microphone on secure origins, and the board serves plain
HTTP. Playback works regardless. For the mic, either:

- Chrome: add `http://esp32-agent.local` (or the IP) at
  `chrome://flags/#unsafely-treat-insecure-origin-as-secure`, relaunch; or
- switch **Chat page audio** to the desk hardware under Settings › Voice.

The page detects the missing API and shows these instructions itself.

## Editing the page

The UI source is in `web/` — `index.html`, `styles.css`, and `js/*.js` (one file
per area: `core`, `nav`, `status`, `voice`, `chat`, `settings-*`, `main`).
`Page.h` is **generated** from it as a gzipped `PROGMEM` array.

```bash
python3 tools/preview.py      # live preview at http://127.0.0.1:8765 with mock APIs
python3 web/build.py          # regenerate Page.h after editing web/
python3 web/build.py --check  # fails if Page.h is out of date
```

Build, then compile and upload as usual — the page is part of the firmware, so
a page change always means a reflash. Details in [tools/README.md](tools/README.md).

## Security notes

Settings — Wi-Fi password, Agora tokens, speech keys — are stored in the board's
NVS flash, unencrypted, and the board's local HTTP API has no login. Keys are
write-only (never sent back to the page) and Agora tokens are dropped from RAM
after each exchange, but anyone with physical access or on the same Wi-Fi should
be treated as trusted. Use a dedicated Agora token you can revoke rather than an
admin key, and consider ESP32 flash encryption if the device leaves your desk.

## Layout

```
Esp32Agent.ino   setup/loop; wiring and board-settings notes
Board.h          pins and limits
Config.*         NVS-backed settings (Wi-Fi, agents, voice)
Portal.*         soft-AP, captive DNS, Wi-Fi join/scan, mDNS
WebUi.*          HTTP routes and JSON API
ChatClient.*     post to Agora, poll for the agent's reply
Feedback.*       LED and buzzer patterns
Button.*         debounced push button (talk, mute, dial switch)
Dial.*           KY-040 rotary encoder: interrupts, detent counter, switch
Quadrature.*     Gray-code state table that turns edges into detents
AgentDial.*      dial clicks -> hands-free agent, beeps, deferred save
Mic.*            I2S mic task (core 0): gain, PSRAM ring, feeds VAD + wake word
Wake.*           wake word engine (TFLite Micro + audio frontend)
WakeModel.h      GENERATED model bytes + manifest (tools/wake/make_model_header.py)
Vad.*            energy voice-activity detector and hands-free endpointing
WakeText.h       strips "hey jarvis"/"hey agora" from transcripts
Audio.*          recordings (from the mic ring) and speaker playback
Speech.*         Groq / OpenAI transcription and text-to-speech
VoiceFlow.*      push-to-talk / wake-word state machine, mute button, listening LED
Json.h, Wav.h    tiny parsers/writers
Page.h           GENERATED gzipped web page
web/             page source + build.py
src/microfrontend vendored TFLM audio microfrontend (Apache-2.0)
tools/preview.py local mock server for the page
tools/wake/      wake model sources, header generator, training notes
```
