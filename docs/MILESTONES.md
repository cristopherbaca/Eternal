# Implementation log

Each milestone is a buildable commit. Screenshots come from the actual framebuffer with a monospace font.

## 1 — Terminal

Truecolor framebuffer, UTF-8, differential frame writes, alternate screen, raw input, SGR mouse, size polling, POSIX/Windows backends and restoration. Headless capture allows repeatable validation.

Validation: CMake build and 120-frame headless render. Reference comparison: matching navy canvas and pale glyphs; geometry, lighting and HUD follow in subsequent milestones. Run `./build/eternal_rpg` (`Q` exits).

## 2 — World

Seeded trees, dense animated grass, flowers, diagonal cobbles and arch ruins. Tilted projection and depth buffer, WASD collision movement, smooth camera and zoom. Simulation runs at a fixed 60 Hz. Run `./build/eternal_rpg`; scroll or `+/-` zooms.

Comparison with forest/grass frames: conifers and rounded foliage now have different silhouettes; castle arches and banners remain legible at 200×60. Biggest remaining differences: flat ambient colors and grass without displacement. Lighting follows in milestone 3.
