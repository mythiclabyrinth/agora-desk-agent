# Desk agent UI preview

Run `python3 tools/preview.py`, then open http://127.0.0.1:8765/.
This reads the current embedded HTML from `Page.h` and serves isolated demo APIs.
It never forwards credentials, changes settings, or sends messages to the board.
Demo saves last only until the preview process stops.

- `/?scenario=offline`: board reachable, Wi-Fi disconnected.
- `/?scenario=empty`: no configured agents.
- `/?scenario=scan-empty#settings`: Wi-Fi scan returns no networks.
- `/?scenario=scan-error#settings`: Wi-Fi scan fails; manual entry remains available.
- Wi-Fi scanning and joining use demo data only.
- Send a message containing `preview-error` to simulate a failed send.
- Other messages receive an explicitly labeled demo response after four seconds.

The production page uses the existing firmware APIs unchanged. Compile and upload
`Esp32Agent.ino` with your board's existing ESP32-S3 settings to update the device.
The browser preview alone does not change the firmware at 192.168.0.113.
