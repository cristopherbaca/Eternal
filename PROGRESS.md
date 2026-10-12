# Living-world expansion

Budget visibility: this session exposes no account usage/context status command. Remaining usage cannot be measured; no estimates are claimed.

Constraints: preserve the existing renderer, combat, class definitions, HUD/menu style, and existing saves. New systems are additive. All generation must be stable for a 12-digit seed. Minimap is included.

## Phases
1. **Complete (first playable slice)** — deterministic named points of interest, landmark silhouettes, connected roads/clearings, discovery/fog, minimap, pannable world map, debug location listing, save migration.
2. **Partially complete** — enterable buildings/dungeons, bounded local simulation, day/night/weather, ambient animals.
3. **Complete (vertical slice)** — generated NPC identities/schedules, dialogue, trade/services; structured history/books and factions.
4. **Partially complete** — quests and a playable town → dungeon → reward loop, quest/lore log and persistent state.
5. **Partially complete** — broader crime, skills/perks, crafting, random equipment affixes and additional quest templates. These are not implemented by the existing game.

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

## Phase 2a result: interiors
- F at a site's door/banner enters a separately generated interior. Peaceful buildings have furniture, torches and a rest point; hostile locations have connected rooms/corridors, traps, two guardians including a stronger warden, and a one-time reward chest. F at < exits. Quarter-second fade uses the existing framebuffer.
- Existing combat is reused, overworld encounters are suspended/restored, and minimap switches to the local floor plan. Save v3 preserves current interior, outdoor return state and cleared chests; v1/v2 still load.
- 174 checks pass, including connected floor plans, peaceful/hostile entry, guarded and non-repeatable loot, exit restoration and saving/loading inside a cleared dungeon.
- Still partial: interiors use a shared family of connected layouts; furniture has no physics. Day/night/weather/wildlife and richer per-type architecture remain. Next: clock-driven population, dialogue/trading, and seeded lore/quests as a playable vertical slice.
- Budget re-check: usage/context status remains unavailable; remaining allowance unknown.

## Phase 3 result: people, lore and dungeon work
- Added data tables for names/cultures, twelve jobs/factions, personalities/dialogue, 24 structured historical events and quest titles. Every inhabited site deterministically generates twelve identities; only residents of nine nearby sites are simulated (maximum 108). Far residents use schedule destinations on arrival.
- Y talks to nearby NPCs; topics include personality, rumors tied to real locations, kin/friends, dungeon work, trade, rest and hiring. Merchants/smiths/alchemists/innkeepers buy/sell with persistent gold; hired companions reuse existing party AI. J opens quests, L learned lore. F at ? in interiors reads an inscription and reveals the referenced historical place.
- A repeatable giver → marked hostile site → guarded chest → giver reward loop works. Rewards affect faction reputation. The main journal follows a scholar's historical relic lead, then the existing Dragon encounter. All current side-quest titles use this clear-and-recover objective; escort/rescue/stealth/etc. are not implemented.
- Save v4 persists generated history text, learned lore, NPC balances/recruitment/current nearby positions, quests and reputation, and loads v1–v3. `--world-report` now lists history and residents as well as sites.
- 189 checks pass, including NPC identity/schedules, varied histories, interaction, the full dungeon/reward loop, repeat work, trade and living-world persistence.
- Limits: social relationships are generated references, not autonomous relationship simulation. Shops have renewable generated stock; inn/service actions are lightweight. No crime/jail, perk tree, crafting or random-affix items yet. Weather currently influences schedules but is not visually rendered. These remain explicit next pieces.
- Budget re-check: no status API is exposed; remaining allowance unknown.

## Phase 2b result: environment and ambient life
- Added deterministic frontier desert/mountain/coast biomes and winding river channels while retaining the original origin biomes. Existing glyph rendering and combat remain intact.
- The saved world clock drives day/night brightness, clear/rain/snow/fog/storm cycles, visibility and NPC shelter schedules. Minimap shows time/weather. Deer flee nearby players/wolves, birds move above the ground, and some existing streamed enemy encounters are wolves using normal combat AI. Ambient wildlife is capped at 18; it regenerates on streaming rather than persisting every animal.
- Added a three-line recent-event feed on roomy HUDs without changing the existing panels or menu style.
- Tests cover added climates, deterministic clock/weather, bounded wildlife and visible day/night differences. Terrain remains streamed; atmospheric effects are a lightweight framebuffer pass.
- Limits: weather is regional ambiance rather than a fluid simulation; wolves use existing hostile combat AI, not an ecological food chain. Roads do not yet form an authored political trade network.
- Budget re-check: remaining usage remains unavailable. Next: integration/performance checks and documenting exact implemented versus remaining scope.

## Phase 4 result: lightweight progression and civic consequences
- U opens the additive mastery/crafting panel. Successful ability use earns per-class mastery; every 20 casts yields a talent point, spent on capped vitality/power/armor talents. Class switching retains those class-specific ranks without stacking bonuses.
- G gathers ore/herbs/food once per chunk per game day. Four data-driven recipes cover smithing, alchemy, enchanting and cooking. Inventory, trade and chests respect a 60-unit carrying capacity (existing item categories supply weights).
- Dialogue offers explicit theft. Nearby witnesses create a town bounty and reduce faction reputation; guards pursue via existing combat AI. A guard accepts a fine or a timed jail/XP penalty. Faction standing labels advance from outsider to ally/champion through quest rewards.
- Save v5 persists talents, practice, materials, harvested areas, bounties and the event feed. Prior save versions are migration-tested.
- Limits: these are basic systems, not full Skyrim subsystems. Talents are three tracks per class, recipes are four explicit recipes, enchanting creates a charm, and crime currently covers theft rather than arbitrary assault/property ownership. Random rarity/affixes, autonomous social relationships, and escort/rescue/stealth quest mechanics remain unimplemented. No claim is made that the whole attached design is complete.
- Budget re-check: remaining usage is still not exposed. Next: final end-to-end, native layout, performance and documentation checks; no new large subsystem will start before those pass.

## Integration result
- Fixed rumor-only fast travel, settlement door reach and recruited-NPC interaction labels. Added distinct POI silhouettes while retaining existing rendering.
- 213 gameplay checks pass; native Metal checks cover dialogue, job acceptance and progression panel in addition to movement, focus, pause and mouse input. Terminal smoke passes input, resize, signal restoration and backpressure.
- Native 240-frame run at a 60 FPS target measured 58.95 submissions/s, 4.52 ms mean work and 5.72 ms p95. This is submission timing, not verified display refresh.
- Updated living-world controls and implementation limits. Next: delivery and escorted-travel jobs with persistence. Remaining usage unavailable.
