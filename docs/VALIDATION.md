# Validation status

## Adventure creation and persistent saves

155 core checks pass, including 12-digit seeds, high seed bits, level-one empty starts, optional God mode, fresh-instance save/load of inventory/equipment/stats/enemies/loot/discoveries, slot isolation, leading zeros, invalid/truncated save rejection, exact empty-slot selection, and delete confirmation/cancellation. The native AppKit test also checks mixed-case name entry without triggering gameplay shortcuts. GPU captures show the title and creation form in `screenshots/15-adventure-title.png` and `screenshots/16-create-adventure.png`.

Save version 1 uses eight slots with temporary-file replacement and 30-second autosaves. Active particles/projectiles and temporary status effects are intentionally reset on resume. Terrain is regenerated from the full numeric seed and stored generation configuration; this is not a terrain-edit save format.

## Native controls and player animation

Added a focusable game view, explicit main/key window and first-responder setup after application launch, and click-to-focus. Clicks now use their event coordinates rather than the cursor position sampled afterward. Mouse down/up events are consumed by the game without entering default view handling.

137 core checks pass, including distinct idle and walking poses. The native AppKit event test verifies launch focus, Enter, held WD, partial/full release, pause, scaled-menu clicks, world mouse coordinates, casting, and clearing movement on focus loss. It posts events into the application queue; it does not simulate physical hardware.

## Readable Metal interface and compact player

Fixed vertically reversed atlas tile addressing. Verified the actual Metal output with GPU texture readback, rather than the independent framebuffer preview. Menu and gameplay captures are `screenshots/13-readable-menu.png` and `screenshots/14-readable-game.png`. HUD/menu text uses a separate 8×16-point grid; terrain keeps its existing density and both layers compose in one draw call. The player uses compact silver-blue armor without zoom-based glyph repetition.

135 checks pass, including independent UI/world dimensions, transparent UI cells, hotbar placement, and scaled-menu click routing. A 180-frame Metal boss run with one diagnostic PNG capture submitted 56.2 frames/s; CPU work averaged 5.42 ms (p95 5.56 ms). Submission rate is not measured screen refresh.

## Playability and frame pacing (latest)

131 checks pass. Added all four held-key diagonals, direction signs, equal straight/diagonal speed, partial and full release, opposite-key cancellation, stale key-down rejection, and local-player contrast checks. The terminal smoke suite passes; its 120-FPS PTY run reached 119.9 submissions/s after the cadence change. This is not terminal display FPS.

A native 180-frame, 320×100 boss demo recorded 57.132 submitted FPS, 5.055 ms mean CPU work and 5.597 ms p95. The application no longer resets the Metal drawable size unless the window changes size, avoids rendering while the prior GPU command is pending, and uses an accumulated input deadline instead of restarting the pacing interval each loop. The FPS readout updates only for submitted frames. The generic macOS Run task now selects the GPU backend; the user's saved 120-FPS target was reduced to 60, preserving their other preferences.

The updated player/terrain palette was reviewed from an actual framebuffer capture: `docs/screenshots/12-player-contrast.png`. Visible refresh rate and physical keyboard feel still need confirmation on the user's display; automated checks cover the movement state transitions.



## Procedural world and mods

122 checks pass after adding seed determinism, negative chunk coordinates, regeneration after unload, visit-order independence, bounded caches, varied biomes, remote encounters, cooldown-preserving enemy unload/respawn, stable entity IDs, ordered mods, content overlays and invalid mod rejection. The terminal integration suite also passes.

A 300-frame, 320×100 headless run at (4096, -4096) with Wild Frontier averaged 2.761 ms CPU work (2.791 ms p95). A second remote run at (-12000, 8000) averaged 2.703 ms. These are renderer/simulation throughput measurements, not visible terminal FPS. A framebuffer from the modded frontier was inspected.

The earlier fixed 500×500 world described below is superseded by 64-unit streamed chunks, with a 49-chunk cache and at most 100 active mobs. The supported numeric envelope is ±1,000,000 world units. Native/scripted executable mods and persistent edited chunks are not implemented; mods are validated data packs loaded on startup.


## Current single-player build (2026-10-11)

The multiplayer client/server and network integration test have been removed. Current CTest contains one unit executable with **101 passing checks**, including title/pause/settings transitions, paused simulation, powerful testing mode, normal-mode restoration, level thresholds, stat preservation across classes, landmark exploration, distant grass physics, travel, optional AI companions and configuration round trips/rejection.

The POSIX smoke suite passes output backpressure, delayed acknowledgements, input, resize, normal exit and signal/crash restoration. A native 320×100 title-screen run completed 120 submissions in 2.120 seconds (56.611 submissions/s), with zero ANSI bytes; its framebuffer was inspected and is included in the README. A 600-frame headless boss run averaged 2.165 ms CPU work (3.111 ms p95); this is not display FPS.

The world now spans 500×500 units with five discoverable landmarks. Physics and shadows use a moving local grid to avoid scaling per-frame grass simulation with the total world area. Standard sessions start at level 1, solo, with testing powers enabled by default; preferences can override testing mode. Settings persist, but progression/world state does not persist after exit.

The earlier measurements below describe previous checkpoints, including the now-removed networking implementation.


Local validation: macOS ARM64, Apple Clang, C++17, Release build, compatible macOS 26.5 SDK. Last source/build review: 2026-10-11.

## Verified locally

- All three executables build: `eternal_rpg`, `game_server`, and `eternal_tests`. The latest incremental rebuild of `game.cpp` produced no warnings.
- 70 automated checks pass, covering rendering math, input parsing, combat, progression, equipment, boss behavior, snapshot parsing and deterministic replay.
- Earlier loopback integration runs passed with two protocol peers and two actual clients, checking movement expiry, MP authority, chat and party membership. The latest review did not rerun this integration test.
- POSIX PTY checks passed for keyboard/mouse input, resize, normal exit and terminal restoration after SIGINT, SIGTERM and SIGABRT.
- A slow-reader PTY regression exposed dropped output when stdin and stdout share a nonblocking file description. The output writer now waits for space and retries the unwritten tail. After the fix, the test verifies every frame byte arrives; all 70 checks and the existing PTY smoke checks pass. This fixes a cause of incomplete scenes and broken ANSI cursor positioning. Visual confirmation in the user's Terminal remains pending.
- Earlier ASan/UBSan checks and a 1,800-frame boss simulation passed. These preceded the final rendering polish and are not a sanitizer certification of every subsequent change.

## Performance observations

Latest frame-pacing update: 74 checks pass, plus a PTY test that delays terminal acknowledgements by 80 ms and verifies no additional frame is queued before the reply. Input/simulation now run between presentation opportunities; the external-terminal launch targets 30 FPS. This does not establish visible terminal FPS. User confirmation of smoothness on the actual display is still required.

The final 360-frame PTY measurement at 200×60 recorded 103.759 FPS, 1.528 ms mean work time and 2.940 ms p95 work time. A PTY consumes terminal output; it does not measure the user's terminal emulator painting those cells.

The final 1,800-frame headless boss demo took 1.617 seconds, with 0.898 ms mean work time and 1.341 ms p95. Its 1,112.932 FPS is unpaced CPU/encoding throughput, not an interactive FPS claim. Both measurements predate the latest include/formatting and IDE configuration changes, which do not change simulation or rendering behavior.

## Remaining acceptance gaps

- The renderer approximates the recordings. Character/dragon detail, glyph density and optical glow still differ; see the milestone log and screenshots.
- Interactive terminal emulator performance needs measurement on the user's setup.
- Linux and Windows code and CI configuration exist, but successful builds and runtime behavior on those systems have not been verified here.
- Multiplayer is a session-based cooperative prototype. Persistence, accounts, matchmaking and a hosted realm are absent.
- Automated checks exercise selected behavior; they do not establish full gameplay balance or exhaustive multiplayer correctness.

## Editor diagnostics

CMake exports `build/compile_commands.json` for Makefile/Ninja builds. The checked-in VS Code C/C++ configuration reads it so editor diagnostics use the same C++17 flags, include paths, definitions and SDK as compilation. Configure CMake before opening source files. Other editors can use the same database. Visual Studio generators do not emit this database; use native project integration there.

The reported `game.cpp` error did not reproduce in the compiler. Its exact IDE diagnostic is still needed to establish whether the editor configuration resolves it.

## Native Metal backend

A 180-frame live native-window boss demo at 320×100 completed in 3.217 seconds (55.949 submitted frames/s), with 5.318 ms mean CPU frame work and 6.920 ms p95. ANSI output was zero. This measures application submissions, not a display scanout trace. The run exercised runtime Metal shader compilation, glyph atlas creation, drawable acquisition and GPU command submission without an error.

The native backend reuses the existing simulation and full-color cell buffer. Terminal mode is retained. Native input now supplies held movement and immediate key release; the corresponding regression checks bring the unit suite to 76 checks. Visual smoothness, font appearance and mouse alignment still need user confirmation on the target display.
