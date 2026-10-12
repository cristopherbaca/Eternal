# Living-world expansion

Budget visibility: this session exposes no account usage/context status command. Remaining usage cannot be measured; no estimates are claimed.

Constraints: preserve the existing renderer, combat, class definitions, HUD/menu style, and existing saves. New systems are additive. All generation must be stable for a 12-digit seed. Minimap is included.

## Phases
1. **In progress** — deterministic named points of interest, landmark silhouettes, connected roads/clearings, discovery/fog, minimap, pannable world map, debug location listing, save migration.
2. **Next** — enterable buildings/dungeons, bounded local simulation, day/night/weather, ambient animals.
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
