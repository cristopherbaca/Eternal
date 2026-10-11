# Implementation log

Implementation checkpoints are recorded in git. Screenshots come from the actual framebuffer with a monospace font.

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

## 6 — Classes and combat

Five silhouettes and 30 data-driven abilities with MP/cooldowns: fire orbs/embers, fire circles, frost, arrows/multishot/traps, heals/wards/resurrection, melee/taunt/charge, poison/backstab/dash. Swept projectile collision, armor formula, particles and grass impulses. Mouse/arrow crosshair, Tab target, six-slot hotbar, `C` class selection. `./build/eternal_rpg --demo` repeatedly casts real abilities for inspection.

Validation: all 30 casts, MP/cooldown rejection, actual projectile damage, healing and death bounds. Comparison with spell footage: orange orb, trailing embers and warm grass tint are present; glyph-scale fire core is smaller than the high-resolution source. Fire ring/particles are used to retain the larger glow silhouette. Run normally to aim and cast manually.

## 7 — Enemies and progression

Data-driven Skeletons, Hollow Wisps and Grave Sentinels with idle/aggro/chase/attack AI. Damage, death/respawn, regeneration, XP/level-ups, gold, named loot and a 24-item inventory with three equipment slots. `E` collects nearby loot, `I` opens inventory, `J/K` selects, Enter equips; `R` respawns when dead.

Tests verify loot/XP exactly once, collection, non-stacking equipment and respawn. Comparison with spell reference: monsters have visible names and damaged health bars; Rusty Stiletto and Old Sabatons appear as world loot. Party is still stationary at this checkpoint; coordinated movement/healing follows next.

## 8 — Party and Dragon

Healer triages/raises allies, warrior taunts/tanks, mage casts at range; all follow and avoid hostile areas. Large red-brown dragon with flapping wings, three data-driven phases, delayed 1.5s red rings, cone breath, burning patches, wing sweep, adds, enrage, wipe reset and victory equipment/gold. `B` travels to the arena; `./build/eternal_rpg --scene boss --demo` previews the fight.

Validation: phase transitions, delayed telegraphs, wipe reset and victory reward tests. Comparison with boss recording: boss/party HUD, broad wing silhouette, warm spell ground and real green heal numbers are present. At 200×60 wings are a simplified procedural silhouette and source footage has far more detail. Timings and damage are tunable in `data/boss*.txt`.

## 9 — Multiplayer

Optional authoritative TCP server, 60 Hz simulation/20 Hz snapshots, position interpolation, eight players, replicated combat/AI/loot, chat and accepted party invitations. Run `./build/game_server`, then `./build/eternal_rpg --connect 127.0.0.1:7777 --name Senje` in another terminal. Enter opens chat; `/who`, `/invite ID`, `/accept` manage a party.

Validation: real loopback server with two protocol peers and two terminal clients; checked movement timeout, MP authority, chat delivery, group membership and rendering. Snapshot round-trip/truncation tests pass. Visual comparison: online and offline use the same renderer/HUD; network effects are sampled to bound bandwidth, so dense particle trails are less smooth. This is a cooperative session prototype, not a persistent MMO service. See [network details](NETWORK.md).

## 10 — Polish and profiling

Added terminal-height camera scaling, canopy shadows, distinct skeleton silhouettes, revised shoreline spawns, piercing projectile hit tracking, CLI help, performance metrics, screenshots and build/control documentation. Run `./build/eternal_rpg --scene boss --demo` or `./build/eternal_rpg --scene water`; use a larger terminal grid for finer geometry.

Validation: 70 checks pass; PTY and headless profiling results and their limits are recorded in [validation status](VALIDATION.md). Comparison with the Dragon and water frames: broad flapping wings, phase/party HUD, ground spell lighting and dim reflected silhouettes are present. The reference still has finer object geometry and richer glow. This checkpoint delivers a playable prototype; exact visual matching and full cross-platform acceptance remain open.
