# EternalRPG

A single-player ASCII RPG in C++17 with a seed-based, streamed procedural world. Explore Ashenmere, Frostwood, Ember Hills, Reedlands and Dragon Hollow; defeat enemies, discover landmarks, earn XP and fight the Dragon. The networking client, server and multiplayer source have been removed.

![Single-player introduction](docs/screenshots/15-adventure-title.png)

On this Mac, select **EternalRPG — GPU Window (recommended)** in VS Code's **Run and Debug** dropdown and click **▶**. Press **Enter** on the introduction screen to start. The generic **EternalRPG: Run** task also launches the GPU window on macOS. The explicitly named external-terminal launch remains available. Use **Run → Run Without Debugging** for normal play.

```sh
cmake -B build
cmake --build build
./build/eternal_rpg --window  # native Metal window, macOS
./build/eternal_rpg           # terminal mode
```

The game, world, menus, settings and progression are C++. The macOS window uses a small Objective-C++ bridge to Apple's AppKit/Metal APIs; it has no Python, JavaScript or game-engine runtime. Terminal backends are provided for macOS, Linux and Windows. Only macOS has been tested locally; CI is configured for all three. On Windows, use `cmake --build build --config Release` and run `build\Release\eternal_rpg.exe`.

New adventures have **God mode off by default**. Enable it in character creation or Settings for easy exploration: invulnerability, unlimited mana, 12× ability damage and 0.12-second ability cooldowns. New characters start at level 1 with zero gold, an empty inventory and no equipped gear. Demo/headless runs retain testing defaults; `--normal` disables them. Testing mode doesn't replace your class or level and can be toggled during a session.

| Control | Action |
| --- | --- |
| Enter | Start the adventure / choose a menu item |
| Esc / P | Pause, resume, or return from settings |
| W/S or arrows in menus | Select an option |
| A/D or Left/Right in Settings | Change selected setting |
| WASD | Move |
| Mouse / arrow keys in game | Aim |
| Click / Space | Primary ability |
| 1–6 | Cast abilities |
| Tab | Cycle targets |
| M | World map |
| T on map | With testing powers enabled, jump to new procedural wilderness |
| 1–5 on map | Travel to discovered landmarks; testing mode unlocks all destinations |
| I, then J/K and Enter | Open inventory, select and equip/unequip |
| E | Collect nearby loot |
| C, then 1–5 | Change class outside combat (anytime with testing powers) |
| + / - / scroll | Camera zoom |
| B | Travel to Dragon arena |
| R | Respawn after death |
| H | Controls overlay |
| Q | Exit |

Pause offers **Resume, Settings, World Map, Save Now, Save and Return to Title, and Save and Exit**. Menus freeze combat, AI, cooldowns and world time. Returning to title saves the current adventure.

Settings include testing powers, optional AI companions (off by default), frame-rate target, native-window detail, camera zoom, name labels, control hints and class. Settings are saved on a normal exit to `.eternalrpg-settings.txt` in the working directory. Character/world progress is stored in eight independent save slots under `saves/` in the working directory. Saves use a temporary file followed by a rename, and autosave every 30 seconds of play as well as on normal exit. Class selection is unavailable during normal combat. Native detail changes glyph resolution; terminal mode takes its grid size from the terminal window.

The world is generated from **seed + coordinates**, using smooth value noise for biomes/lakes and coordinate hashes for trees and enemies. The old 500×500 boundary is gone. Terrain streams in 64×64 chunks; only a 7×7 neighborhood is cached, so exploration does not keep expanding memory usage. Unloaded terrain regenerates identically for the same seed and mod set. The five starting landmarks and ruins remain near the origin; the frontier continues beyond them with forests, grassland, ember hills, wetlands, lakes and roads.

Enemy spawns are generated around the current chunk. Distant encounters unload, new regions create new encounters, and defeated enemies respawn after a configurable interval. At most 100 mobs are active at once. Kill cooldowns survive leaving and returning during the cooldown, but distant surviving enemies reset when regenerated. Dropped loot beyond 200 units is discarded. This provides continuing encounters rather than a finite initial enemy population.

This is an effectively large procedural world, not mathematical infinity: the current float-coordinate implementation supports ±1,000,000 world units per axis. There is no voxel building/mining system. Terrain edits such as building or mining are not implemented. **M → T** jumps farther into the generated frontier in testing mode; **M → 1** returns home. The map now follows your position instead of displaying the entire world.

Landmarks award 150 discovery XP once per save (the starting landmark is already known). Enemies grow stronger with distance (enemy level caps at 100) and award level-scaled XP. Each player level costs `100 + 25 × current level` XP and grants **+8 maximum HP and +2 power**. These bonuses survive class changes.

Mods are loaded from `mods/`. Add a folder name to [`mods/enabled.txt`](mods/enabled.txt) and restart, or run:

```sh
./build/eternal_rpg --window --mod wild_frontier
./build/eternal_rpg --list-mods
./build/eternal_rpg --window --no-mods
./build/eternal_rpg --window --mod wild_frontier --position 4096 -4096
```

The included **Wild Frontier** example is disabled by default. It changes forest/lake generation and enemy density, and adds a Frontier Stalker. Mods can override generation parameters and the existing content tables. These are declarative data packs, not arbitrary executable plugins or scripts. See the [mod creation guide](docs/MODS.md) for the format, load order, supported fields and limits.

Movement in the GPU window reads the complete held-key state: WA, WD, AS and SD move diagonally at the same speed as straight movement. Releasing one key retains the other direction, and opposing keys cancel. The local player wears compact silver-blue ASCII armor. Its silhouette stays small at every zoom level, with idle breathing and a four-frame walking cycle (alternating feet, arm swing and body bob). The silhouette stays visible when scenery overlaps it; muted green foliage and warm enemy accents distinguish terrain from characters.

The Metal backend renders the same RGB glyph framebuffer using a cached Menlo atlas and one GPU draw call. It avoids ANSI terminal output and handles held movement keys and key releases directly. Use **Comfort**, **Detailed**, or **Dense** in Settings to balance readable text against glyph density. The displayed `Sent fps` measures submissions, not verified display scanout.

Terminal mode needs a truecolor terminal and a monospace font. About 200×60 cells is a useful starting point. More rows/columns improve detail but increase terminal work; 320×100 is better suited to the GPU window. Terminal movement depends on keyboard repeat because portable terminals do not report key releases.

```sh
./build/eternal_rpg --window --scene boss --demo
./build/eternal_rpg --normal --scene forest
./build/eternal_rpg --benchmark --scene boss --demo --frames 600 --width 320 --height 100
ctest --test-dir build --output-on-failure
python3 tools/terminal_smoke.py build/eternal_rpg  # optional POSIX developer check
```

`--demo` and headless runs skip the introduction and saved preferences for reproducibility. Use `--seed`, `--name`, `--class`, `--fps`, `--scene` and `--data` for startup overrides. An explicit class/FPS overrides the remembered setting. Data tables in `data/` define five classes, 30 abilities, enemies, items and boss phases. Run `--help` for options. Python and the optional macOS capture tool are development utilities, not game dependencies.

See [validation](docs/VALIDATION.md) and the [historical milestones](docs/MILESTONES.md). Original reference screenshots in that log predate the solo conversion. The visuals still approximate the reference videos; this remains a playable prototype rather than a finished commercial RPG.

This workspace uses a compatible macOS 26.5 SDK (`-DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk`). The VS Code build task currently points at `/private/tmp/eternalrpg-build-tools/cmake/data/bin/cmake`; update its command if that temporary installation is removed. CMake exports a compilation database used by the included VS Code C/C++ configuration. Use the whole-project launch, not Code Runner's single-file Run Code button.

The Metal window renders menus, status panels and the hotbar on a separate 8×16-point text grid, independently of terrain detail. For renderer diagnostics, `ER_CAPTURE=/tmp/eternal.png ./build/eternal_rpg --window --frames 2` captures the first actual GPU frame (including the glyph atlas and interface).

On macOS, `./build/eternal_native_input_tests` opens a short-lived test window and checks AppKit event routing, held WASD, releases, pause/resume clicks and mouse casting. It requires a graphical desktop.

Create a world from **New adventure**: enter a world name and player name, choose Warrior/Mage/Archer/Cleric/Rogue, edit the generated 12-digit seed if desired, and toggle God mode. Enter or click a text field to edit it; Backspace erases and Enter finishes. Both halves of the numeric seed contribute to terrain generation, and leading zeros remain visible in the world map.

**Continue / manage saved worlds** lists eight slots. Select a world to continue or delete it; deletion requires confirmation and defaults to Cancel. Saves retain level/XP, health/mana, gold, inventory, equipped items, position, discoveries, nearby enemies, boss progress, dropped loot, respawn deadlines, seed and generation settings. Active projectiles, particles and temporary status effects reset when loading. Use the same game data and mod versions when continuing; mismatched mod names or item tables are rejected. Back up the `saves/` folder to transfer adventures.

Equipment slots are **Weapon, Clothing, Charm**. Defeat enemies and press E near loot; open I, select with J/K and press Enter to equip or remove an item. Additional drops include a linen tunic, leather coat and iron sword. Equipment affects power/armor; the player sprite distinguishes bare hands and plain clothing from equipped gear. This implements the adventure/save/equipment foundation; shops, quest chains, crafting and skill trees are not implemented yet.
