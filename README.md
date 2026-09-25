# Agora Desk Agent

**One Agora. Many agents. Now within reach.**

Agora brings people and AI agents together in shared rooms. Agora Desk Agent
brings that workspace to your physical desk: turn a dial to choose Claude,
Cursor, or Codex, press to talk, and hear the reply. You can also type a message
from the web page hosted on the device.

Built on an ESP32-S3 (N16R8), the desk agent posts an `@mention` into your
[Agora](../../agora) channel and brings the agent’s reply back to the page or
speaker. Lights, a buzzer, and an optional display keep you aware of its progress.

The bridges stay on your computer. The board is a thin client: it never runs an
LLM and never holds a model key; it holds an Agora token per agent and, if you
use voice, a Groq and/or OpenAI key for speech.

## What it does

- **Web UI on the board** — Overview, Conversations, Settings (Wi-Fi, Agents,
  Voice). Served from flash, no CDN or internet needed to load. The warm light
  theme is the default; the header toggle switches to Agora’s dark palette and
  remembers your choice in this browser.
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
- **Wake word (hands-free)** — say "Hey Jarvis", wait for two beeps, speak, and
  pause. The model runs on the board (microWakeWord on TensorFlow Lite Micro);
  an energy VAD ends the recording when you stop talking and drops it if you
  said nothing. A **mute button** (or Settings › Voice) switches the wake word
  off and on, and a **blue LED** is lit whenever the board's mic is listening.
  The wake word is deaf while the board beeps or speaks. To train a "Hey Agora"
  model, see [tools/wake/README.md](tools/wake/README.md).
- **Hands-free agent and the dial** — the talk button and the wake word reach
  one agent, set under Settings › Voice › Devices or with a KY-040 rotary dial.
  The dial steps through the *configured* agents (clockwise = next) and, once
  the knob rests, beeps the agent's place: one for Claude, two for Cursor, three
  for Codex. A short press replays it. With one agent configured the dial just
  ticks; with none it gives the error beeps. A turn mid-recording applies to the
  next message.
- **Status display (optional)** — a 16x2 I2C LCD shows the hands-free agent,
  whether the wake word is listening (or muted), and the Wi-Fi address; during
  an exchange it shows listening, thinking, waiting (with a timer) and
  speaking. Short notices cover replies landing, errors, dial turns, mute, the wake word and Wi-Fi changes. The backlight goes
  off after a minute at rest and comes back on any activity.
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
| Mute button | GPIO 46 → button → GND (internal pull-up) |
| Blue listening LED | GPIO 18 → 47–100 Ω → LED → GND |
| KY-040 rotary dial | CLK 9, DT 10, SW 3, **+ → 3V3 (not 5V)**, GND → GND |
| INMP441 microphone | SCK 12, WS 11, SD 13, L/R → GND, VDD 3V3 |
| MAX98357A amplifier | BCLK 16, LRC 15, DIN 17, VIN 5V, 4 Ω speaker on +/− |
| 16x2 LCD, PCF8574 I2C backpack | SDA 8, SCL 7, VCC 5V, GND |

Power the KY-040 from 3V3: its pull-ups go to "+", and the ESP32-S3's GPIOs are
not 5 V tolerant. If clockwise selects the previous agent, set `DIAL_REVERSE` in
`Board.h`; if one click moves two agents or every other click is ignored, adjust
`DIAL_STEPS_PER_DETENT`. The blue LED needs a small resistor (47–100 Ω) or it
barely glows. GPIO 46 is a strapping pin: keep the mute button a plain button
to GND, with no pull-up resistor, or uploads fail. The LCD backpack needs 5V
for contrast, which also puts its I2C pull-ups at 5V: remove them and fit
4.7 kΩ from SDA and SCL to 3V3, or use a BSS138 level shifter. The board looks
for the backpack at 0x27 and 0x3F; turn its contrast pot if the screen is lit
but blank. All of this hardware is optional; the page works without it.

## Build and flash

Arduino IDE 2 with the `esp32` core 3.x. Board settings:

- Board: **ESP32S3 Dev Module**
- PSRAM: **OPI PSRAM** (recordings and speech buffers live there)
- Flash Size: **16MB**
- Partition Scheme: **16M Flash (3MB APP/9.9MB FATFS)**

Library Manager: install **LiquidCrystal I2C** by Frank de Brabander (for the
LCD; the sketch needs it to compile even without one attached).

Open `Esp32Agent.ino`, compile, upload. Watch the Serial Monitor at 115200 for
the setup-network address and, once joined, the board's IP.

If you changed anything in `web/`, run `python3 web/build.py` first (see
[Editing the page](#editing-the-page)).

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
   choose providers/models, and pick the hands-free agent under Devices.

### Browser microphone and HTTPS

Browsers only expose the microphone on secure origins, and the board serves plain
HTTP. Playback works regardless. For the mic, either:

- Chrome: add `http://esp32-agent.local` (or the IP) at
  `chrome://flags/#unsafely-treat-insecure-origin-as-secure`, relaunch; or
- choose **Desk device** under Settings › Voice › Devices to use the board's mic.

The page detects the missing API and points to the desk option.

## Editing the page

The UI source is in `web/`; `Page.h` is **generated** from it as a gzipped
`PROGMEM` array.

```bash
python3 tools/preview.py      # live preview at http://127.0.0.1:8765 with mock APIs
python3 web/build.py          # regenerate Page.h after editing web/
python3 web/build.py --check  # fails if Page.h is out of date
```

A page change always means a reflash. Details in [tools/README.md](tools/README.md).

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
Display.*        16x2 I2C LCD: drawing task, diffed redraws, notices, backlight
StatusScreen.*   what the LCD shows: polls the desk's state, raises notices
Screens.h        every LCD layout, as pure functions
LcdText.h        LCD text helpers (ASCII-only, fit, align, scroll)
Glyphs.h         custom LCD characters
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
