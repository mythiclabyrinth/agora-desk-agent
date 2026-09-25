# Embedded platform marks

These are identifying platform assets, not redrawn approximations. The UI embeds
one copy of each mark in the SVG symbol sheet in `Page.h`, then reuses it with
`<use>`. There are no runtime image downloads. The ESP32 serves the whole page
from `PROGMEM` via `WebServer::send_P`.

Sources retrieved September 24, 2026:

- **Claude:** https://claude.com/icon.png, the official 32 × 32 transparent
  PNG linked from https://claude.com/. Embedded as a PNG data URI inside a symbol;
  displayed at 19–32 CSS pixels. Original retained as `claude.png`.
- **Cursor:** https://cursor.com/brand → official brand asset archive,
  `General Logos/Cube/SVG/CUBE_2D_LIGHT.svg`. Original retained as `cursor.svg`.
  The path, viewBox, and official dark fill are preserved in the embedded symbol.
- **Codex:** official OpenAI mark from https://developers.openai.com/codex/.
  https://openai.com/codex/ currently uses the ChatGPT/OpenAI knot alongside the
  Codex title. Original SVG retained as `openai.svg`; its geometry is preserved.

Brand ownership remains with Anthropic, Anysphere, and OpenAI respectively.
The files here are source references; Arduino does not need to upload a separate
filesystem image or serve this directory.

## Agora identity

`agora.png` is the existing 64 × 64 app icon from the local Agora repository,
`crates/agora-desktop/icons/64x64.png` (copied September 25, 2026). It is embedded
once as the `i-agora` SVG symbol in `web/index.html` and reused in the sidebar
and desk illustration. The charcoal background, violet accent, and teal highlight
follow `agora/web/src/styles.css`. No runtime asset requests are made.
