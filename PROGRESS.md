# Living-world expansion

Budget visibility: this session exposes no account usage/context status command. Remaining usage cannot be measured; no estimates are claimed.

Constraints: preserve the existing renderer, combat, class definitions, HUD/menu style, and existing saves. New systems are additive. All generation must be stable for a 12-digit seed. Minimap is included.

## Phases
1. **Complete (first playable slice)** — deterministic named points of interest, landmark silhouettes, connected roads/clearings, discovery/fog, minimap, pannable world map, debug location listing, save migration.
2. **Next / starting** — enterable buildings/dungeons, bounded local simulation, day/night/weather, ambient animals.
3. **Pending** — generated NPC identities/schedules, dialogue, trade/services; structured history/books and factions.
4. **Pending** — quests and a playable town → dungeon → reward loop, quest/lore log and persistent state.
5. **Pending** — broader crime, skills/perks, crafting, random equipment affixes and additional quest templates. These are not implemented by the existing game.

## Verification / running
- Build: `/private/tmp/eternalrpg-build-tools/cmake/data/bin/cmake --build build -j 6`
- Core checks: `./build/eternal_tests`
- Native controls: `./build/eternal_native_input_tests` (graphical desktop required).
- Terminal: `python3 tools/terminal_smoke.py ./build/eternal_rpg`
- Play: `./build/eternal_rpg --window`; normal new adventures default to God mode OFF.
- Commits are made after each compiling/tested phase. This file records actual completion, limitations and next steps.

## Phase 1 result
- Added 18 data-driven POI types/names/purposes, a guaranteed nearby village and a capital, nine-site streaming, settlement clearings and connected road approaches. Sites are spaced on 256-unit cells (~32 seconds at normal movement speed along a straight road; obstacles and water vary travel time).
- Minimap shows north, player, nearby enemies and discovered places; nearest landmark/distance below. M is a pannable/zoomable atlas with fog, Tab selection and Enter fast travel. Small terminals below 85x30 omit minimap to preserve HUD readability.
- Save v2 preserves fog/discovery and still loads v1. `--world-report --seed 123456789012` lists generated sites and coordinates; `--position X Y --demo` inspects them.
- 163 checks pass; build clean; actual Metal capture inspected. 320x100 headless work ~2.5 ms/frame. A short native capture run submitted 51.3 fps (includes PNG readback); this is not a claim of 60 displayed fps.
- Partial Priority 1: biome expansion, enterable interiors, climate and wildlife remain for subsequent pieces. Structures currently share a simple building silhouette; richer per-type silhouettes remain.
- Budget re-check: no usage/context status API available; remaining allowance unknown.
