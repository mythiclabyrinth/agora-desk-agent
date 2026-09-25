# Desk agent UI preview

Run `python3 tools/preview.py`, then open http://127.0.0.1:8765/.
It assembles the page from `web/` on every request (so edits show on reload)
and serves it gzipped, as the board does. The demo APIs are isolated: nothing is
forwarded to a board, and demo saves last only until the process stops.

- `/?scenario=offline`: board reachable, Wi-Fi disconnected.
- `/?scenario=empty`: no configured agents.
- `/?scenario=scan-empty#settings`: Wi-Fi scan returns no networks.
- `/?scenario=scan-error#settings`: Wi-Fi scan fails; manual entry remains available.
- `/?scenario=voice`: watch a talk-button conversation land in the chat.
- `/?scenario=wake`: wake listening switches on, the score meter (Settings › Voice ›
  Devices) climbs past the cutoff line, “Hey Jarvis” is heard, the VAD ends the clip, and the exchange lands in
  the chat with the “Heard …” banner.
- `/?scenario=dial`: Cursor counts as configured too, and the dial moves the hands-free
  agent (`voice.target_agent`) between Claude and Cursor every three seconds. The
  Hands-free agent picker follows it (unless it has focus), and so does the Overview's
  “hands-free” badge.
- Wi-Fi scanning and joining use demo data only.
- Send a message containing `preview-error` to simulate a failed send.
- Other messages receive an explicitly labeled demo response after four seconds.
- Voice: save any `gsk_…` key under Settings › Voice; transcription returns a fixed
  sentence and speech is a short two-tone chime. `localhost` counts as a secure origin,
  so the browser microphone works here without any flag.

## Editing the page

The page source is `web/`:

| File | What it holds |
| --- | --- |
| `web/index.html` | Markup, the SVG symbol sheet, and the ordered `<script src>` list |
| `web/styles.css` | The one stylesheet |
| `web/js/core.js` | Helpers, storage, `api()`, page-wide state |
| `web/js/nav.js` | Tabs and settings sub-tabs |
| `web/js/status.js` | Chat log store, home cards, `/api/status` polling |
| `web/js/voice.js` | Speaker toggle, mic button, browser vs desk audio |
| `web/js/chat.js` | Conversation view, composer, `sendMessage`, `waitFor` |
| `web/js/settings-agents.js` | Settings › Agents |
| `web/js/settings-voice.js` | Settings › Voice |
| `web/js/settings-wifi.js` | Settings › Wi-Fi |
| `web/js/main.js` | Boot and polling |

The scripts share one global scope and are concatenated in the order listed in
`index.html`, so keep top-level `let`/`const` declarations ahead of the files that
use them at load time.

`Page.h` is **generated**. After editing anything in `web/`:

```bash
python3 web/build.py          # inlines CSS + JS, gzips, writes Page.h
python3 web/build.py --check  # fails if Page.h is stale (preview also warns)
```

Then compile and upload `Esp32Agent.ino`.

## Wake word model

`tools/wake/` holds the model the firmware runs (`models/*.tflite` + `.json`) and
`make_model_header.py`, which generates `WakeModel.h`. See
[tools/wake/README.md](wake/README.md) for training a "Hey Agora" model.
