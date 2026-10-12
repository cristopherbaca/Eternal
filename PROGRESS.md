# Living-world expansion

Budget visibility: this session exposes no account usage/context status command. Remaining usage cannot be measured; no estimates are claimed.

Constraints: preserve the existing renderer, combat, class definitions, HUD/menu style, and existing saves. New systems are additive. All generation must be stable for a 12-digit seed. Minimap is included.

## Phases
1. **Complete (first playable slice)** — deterministic named points of interest, landmark silhouettes, connected roads/clearings, discovery/fog, minimap, pannable world map, debug location listing, save migration.
2. **Partially complete** — enterable buildings/dungeons, bounded local simulation, day/night/weather, ambient animals.
3. **Complete (vertical slice)** — generated NPC identities/schedules, dialogue, trade/services; structured history/books and factions.
4. **Partially complete** — quests and a playable town → dungeon → reward loop, quest/lore log and persistent state.
5. **Partially complete** — basic theft/guards, class mastery/talents and four crafting recipes are playable. Broader crime, random equipment affixes and further quest mechanics remain.

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

## Phase 4b result: delivery and escort contracts
- Repeat work from a giver cycles dungeon recovery → letter delivery → escorted travel. Delivery targets a named resident of a real settlement; talking with Y hands over the quest letter. Escort names another resident and renders a gold traveler following on foot.
- Escorts pause near hostile enemies or when the player gets more than 35 units ahead. The map marks their position; both player and traveler must reach the destination. One escort can be active at a time, and fast travel/building entry are disabled until arrival. The home resident is hidden while traveling.
- Journal objectives explain the interaction and point back to the giver after completion. All contracts pay once on return. Save v6 retains quest type, contact identity and traveler position; versions 1–5 still load.
- 228 core checks pass, including recipient identity, rewards, escort movement/threat waiting, transition guards and saved active escort. Native Metal input checks also pass.
- Limits: escorts cannot be injured, have simple collision-aware following (guide them around obstacles), and return to their home schedule after arrival. There is no abandonment/failure UI yet. Rescue/stealth contracts, autonomous social relationships and item rarity/affixes remain unfinished.
- Budget re-check: no usage API is available; remaining allowance unknown. Next phase: escort cancellation/recovery and richer quest outcomes, followed by equipment rarity/affixes.

# Terrain-only improvement plan (2026-10-12)
Budget: no usage/status API is available; remaining allowance unknown.
Scope: terrain generation, projection, movement slope response and biome presentation only. Preserve combat, classes, HUD/menu layout, NPCs and quests. Existing region footer will display generated region names.
1. Height/shape: warped multi-octave elevation, ridges/valleys/plateaus/basins, cached chunks, slope lighting/cliffs, consistent height projection and slope-aware movement. Build, tests, performance, commit.
2. Biomes: height/moisture/temperature blend, eight landscapes, vegetation/rocks/plants/ambient particles and readable water. Build, tests, visual/performance checks, commit.
Reference videos from earlier messages are absent from their supplied Downloads paths; ffmpeg is unavailable. Existing repository screenshots are available for visual comparison.

## Terrain Step 1: height and shape
- Added seed-derived warped four-octave hills, continental basins, ridged mountain ranges and continuous plateau shelves. A global 4-unit lattice provides seamless bilinear heights and slope normals; 49 lazily streamed chunks bound the working set.
- Camera projects world objects and mouse positions through a small height displacement. Terrain uses fixed directional slope lighting, ridge/valley brightness and rock/cliff glyph transitions. Existing landmarks/ruins retain accessible approaches; roads and settlement clearings are exempt from cliff collision.
- Movement slows uphill, handles gentle descents and collision-tests long steps so cliffs cannot be skipped. No combat, class, HUD/menu, NPC or quest source logic changed. One render integration line grounds the existing camera.
- 234 checks pass (six new terrain checks). Optimized 320×100 frontier benchmark: 8.24 ms mean, 8.65 ms p95, versus 14.59 ms mean before removing repeated projection/cache lookups. Inspected native Metal capture.
- Budget remaining: unavailable. Next: blended climate biomes, vegetation/rocks, water readability and biome particles.

## Terrain Step 2: blended biomes and natural detail
- Added continuous height/moisture/temperature biome weights for meadow, deep forest, pine/frost forest, swamp, desert/dunes, rocky highlands, snowy mountain and beach/coast. Legacy landmarks keep their names; existing footer provides the region label without HUD changes.
- Blended ground palettes and coordinate-dithered glyphs; varied grass density, tree species/density, snow caps, reeds/flowers/cacti, boulders and capped pollen/snow/dust/firefly motes. Preserved grass bending.
- Brighter blue-green water, animated ripples, height basins and warped contour river channels preserve existing reflections and reference lake. No erosion/flow simulation.
- 238 checks pass, plus native controls. Native 320×100, 240-frame dune run: 58.90 submissions/s at 60 target, 13.34 ms p95 work. Headless frontier: 10.78 ms mean / 12.19 ms p95. Native captures inspected and stored in docs/screenshots.
- Terrain changes regenerate older worlds under the new algorithm; save schema remains compatible. All edits are terrain, camera integration, tests or documentation.
- Budget remaining: unavailable. Both requested terrain steps are complete; no further gameplay systems were started. See docs/TERRAIN.md for reproducible biome coordinates and limits.

# Real 3D renderer conversion — Step 1 only
Budget: remaining usage is not exposed by this session.
Plan: one world/screen perspective camera and terrain ray casting; bounded chunk meshes with normals; Metal offscreen color/normal/depth attachments; raw preview and a temporary brightness-only ASCII preview using the existing atlas. Keep existing glyph objects as transitional placeholders until Step 4. Preserve gameplay and the crisp HUD.
Validate camera/ray math, mesh seams, native controls, spells/combat/save regression, raw/ASCII captures and timing. Commit this step and wait for “next”; edge/temporal ASCII, shadows, models, grass/reflections belong to later steps.
Reference: earlier supplied video paths are currently absent and ffmpeg is not installed. Existing reference-era repository screenshots are the available comparison images; do not label these as extracted video frames.

## 3D Step 1 result — ready for review
- Added one perspective world/screen camera module, 55-degree initial tilt, right-drag orbit, camera-relative movement, zoom/FOV controls and terrain ray casting matching mesh triangles.
- Added bounded 49-chunk terrain meshes with normals, cached GPU uploads, Metal offscreen color/normal/depth targets and hardware terrain depth testing. Existing glyph objects remain terrain-grounded placeholders pending Step 4.
- Native output provides an initial brightness-only ASCII preview and F8 raw view. F9 displays GPU timings. HUD/menu layout, combat/classes, seed and save format are preserved. Interiors/terminal and --renderer-2d keep the prior renderer.
- 247 core checks and native input/GPU tests pass. Existing combat, spell and save checks remain passing. Native 240-frame result: 59.77 submissions/s, CPU 6.00 ms mean / 7.48 ms p95, terrain GPU 0.313 ms mean, composite GPU 2.357 ms mean.
- Inspected raw/ASCII native captures; saved in docs/screenshots/18-3d-foundation-*.png. References available only as repository screenshots: original video paths are missing. Biggest remaining visual gap is uniform brightness glyph bands; Step 2 addresses edges/stability. Models, shadows, grass and reflections remain later steps.
- Budget remaining: unavailable. Stop here and wait for “next”; do not start Step 2.

## First-person camera correction
- User clarified that the intended view is first-person. Native 3D now defaults to player eye level, immediate positional following, captured mouse look, camera-relative WASD and center-crosshair terrain aiming. Own sprite/name are omitted. Menus/focus loss/exit release capture. --third-person keeps the orbit option.
- Corrected far-plane clipping so the distant Dragon cannot appear across the entire world as a giant glyph overlay. Combat, classes, saves and HUD layout remain unchanged.
- Build succeeds and 253 core checks pass. Native raw/ASCII captures saved under docs/screenshots/19-first-person-*.png. Native input automation could not reliably acquire macOS keyboard focus, so its active-capture/held-key checks are not claimed as passing.
- Visual conversion is still the basic Step 1 ramp with placeholder glyph entities. Next: improve ASCII slope/edge readability, then the remaining renderer stages. Budget remaining unavailable; stop for user review.
- First-person native 120-frame timing: 57.88 submissions/s at 60 target, 2.77 ms mean / 6.05 ms p95 CPU work, 0.218 ms terrain GPU and 2.164 ms composite GPU.

## ASCII density/readability correction — ready for review
- User requested finer, more recognizable ASCII. Restore independent world density (the 3D path incorrectly used the coarse HUD grid), preserving the HUD grid and layouts.
- Add a GPU conversion kernel once per cell: depth/normal contour glyphs, world-anchored near-ground variation with distance fade, brighter foreground and faint source-colored backgrounds for continuous silhouettes. No frame-random noise or screen-history ghosting.
- Validate native shader compilation, raw/ASCII captures, core regression and frame timings before committing. Full object models and temporal reprojection remain later work.
- Budget remaining: unavailable.
- Result: default source grid is 320×100 instead of 160×50 (four times the cells), with 160×50 HUD text. Existing Settings → Window detail controls all three native presets again; F9 displays actual grid dimensions.
- Warning-free build and 253 core checks pass. New focus-independent native renderer checks pass under Metal API validation: all density presets, growing/shrinking GPU buffers, independent HUD size, raw view and 2D fallback. No Metal validation errors.
- 240-frame native ASCII measurement: 58.70 submissions/s at 60 target; CPU work 3.92 ms mean / 5.65 ms p95; terrain GPU 0.417 ms and conversion/composite GPU 2.120 ms. This does not establish sustained 60+ FPS or display scanout timing.
- Inspected matching raw/ASCII captures in docs/screenshots/20-density-*.png against the previous first-person capture. Finer contours and continuous color improve terrain shape; repeated glyph bands, coarse terrain geometry and flat placeholder entities remain. Source videos remain unavailable at the supplied paths; no matching video frame can be shown. Full models and temporal reprojection were not started.
- Budget remaining: unavailable. Stop for user review before the next renderer stage.

## Creature depth/scale and Shift sprint — complete
- User reported creatures visible through mountains and not growing on approach. Add camera/device depth to world cells and terrain depth rejection in Metal; world name labels carry depth while the HUD/crosshair remain overlays. Perspective sprites scale by distance and field of view; nearer objects win overlap ordering.
- Hold Shift with WASD for 1.8× native-window movement speed; keep diagonal normalization, slopes, water/slow penalties and collision handling. Focus/menu state disables sprint.
- Verify camera/depth ordering, sprint/release, native GPU occlusion, screenshots and performance before commit. Remaining usage unavailable.
- Implemented per-cell GPU terrain depth rejection for sprites, spells and world labels in both ASCII and raw modes. Names follow physical sprite height; the HUD/crosshair stay unoccluded. Humanoids, skeletons, wolves and Dragon now use perspective sizing. Near/far clipping and viewport-bounded loops avoid oversized offscreen work. These remain glyph billboards, not full 3D models.
- Warning-free build; 260 core checks pass including distance scaling, camera depth ordering and sprint/release/diagonal normalization. Metal-validation renderer checks pass, including image comparisons proving occluded glyphs reproduce the terrain-only frame and near glyphs remain visible. All density presets, raw view and fallback pass. Hardware Shift/focus behavior still needs interactive desktop confirmation; gameplay input behavior is covered by core checks.
- Inspected ASCII/raw captures at the same location as the previous change, saved as docs/screenshots/21-creature-depth-*.png. The source videos are still unavailable. Models/vegetation remain the largest visual gap, outside this correction.
- Final 240-frame native run: 58.62 submissions/s at a 60 target, CPU 7.06 ms mean / 10.58 ms p95, terrain GPU 0.287 ms, conversion/composite GPU 2.331 ms. No claim of sustained 60+ display FPS.
- Remaining usage unavailable. Ready for user review; no subsequent renderer stage started.
