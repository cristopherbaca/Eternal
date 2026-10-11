# Implementation log

Each milestone is a buildable commit. Screenshots come from the actual framebuffer with a monospace font.

## 1 — Terminal

Truecolor framebuffer, UTF-8, differential frame writes, alternate screen, raw input, SGR mouse, size polling, POSIX/Windows backends and restoration. Headless capture allows repeatable validation.

Validation: CMake build and 120-frame headless render. Reference comparison: matching navy canvas and pale glyphs; geometry, lighting and HUD follow in subsequent milestones. Run `./build/eternal_rpg` (`Q` exits).
