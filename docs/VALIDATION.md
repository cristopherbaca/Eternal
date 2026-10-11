# Validation status

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
