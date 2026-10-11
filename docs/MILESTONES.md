# Implementation log

Each milestone is a buildable commit. Screenshots come from the actual framebuffer with a monospace font.

## 1 — Terminal

Truecolor framebuffer, UTF-8, differential frame writes, alternate screen, raw input, SGR mouse, size polling, POSIX/Windows backends and restoration. Headless capture allows repeatable validation.

Validation: CMake build and 120-frame headless render. Reference comparison: matching navy canvas and pale glyphs; geometry, lighting and HUD follow in subsequent milestones. Run `./build/eternal_rpg` (`Q` exits).

## 2 — World

Seeded trees, dense animated grass, flowers, diagonal cobbles and arch ruins. Tilted projection and depth buffer, WASD collision movement, smooth camera and zoom. Simulation runs at a fixed 60 Hz. Run `./build/eternal_rpg`; scroll or `+/-` zooms.

Comparison with forest/grass frames: conifers and rounded foliage now have different silhouettes; castle arches and banners remain legible at 200×60. Biggest remaining differences: flat ambient colors and grass without displacement. Lighting follows in milestone 3.

## 3 — Lighting and ASCII

Software pixels retain depth, world position, height, normal and material color. Conversion shades with directional ambient light, warm radial sources, luminance ramp, surface edges and vignette. Torch flicker colors nearby stone glyphs. Unit tests cover projection, math, ASCII, falloff, input and diff output.

Run `./build/eternal_rpg` and `ctest --test-dir build --output-on-failure`. Reference comparison: removed uniformly bright peripheral foliage and added warm stone near torches. True optical bloom is approximated by colored glyphs; terminal backgrounds stay navy. Grass displacement and reflected silhouettes remain next.

## 4 — Grass and water

Damped spring grid bends grass radially around movement and recovers over time. Reflection pass mirrors the completed scene at an irregular shoreline, dims it, applies horizontal ripple distortion and ripple rings. Run `./build/eternal_rpg --scene water` or `--scene grass`.

Validation: spring displacement/recovery tests and water capture. Comparison: near-black water now contains dim canopy/player silhouettes rather than bright blue fill. Blade geometry is one terminal cell at the default zoom, so curves are approximated by changing orientation and brightness. Source footage resolves several glyphs per blade.

## 5 — HUD

Outlined player/party panels, actual numeric HP/MP bars, level/gold, party tab, white overhead names, conditional target health bars, floating-text renderer and responsive compact layout. Run `./build/eternal_rpg`; `--width 60 --height 20 --headless --frames 1` verifies a small terminal.

Comparison: panel proportions and red/blue bars match the boss footage layout. Player and party names make the dark figures identifiable. Floating text is wired for real combat events in the next milestone; no fabricated damage/heals are shown. Boss bar rendering exists but awaits a live boss.
