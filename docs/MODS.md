# Modding EternalRPG

The C++ loader reads versioned data packs at startup. No scripting runtime, executable plugin loading, or recompilation is needed for supported changes. Changing the generator's fundamental algorithm or adding a new ability behavior still requires C++ source changes.

## Install and enable

Put each pack in `mods/<id>/`, where `<id>` contains only letters, digits, `_` or `-`.

```text
mods/
  enabled.txt
  my_world/
    mod.txt
    world.txt          # optional generation overrides
    enemies.txt        # optional COMPLETE replacement table
```

Required `mod.txt`:

```text
api 1
name "My World"
```

Put `my_world` on its own line in `mods/enabled.txt`, then restart. Lines beginning with `#` are comments. You can instead append packs for one run with `--mod my_world`. Multiple `--mod` flags are supported. `--mods-dir PATH` selects another pack directory. `--no-mods` disables all packs, including command-line packs; `--list-mods` shows active manifest names and exits. The default root is `./mods`, falling back to the source project's mods folder when launched from another directory.

Enabled-file order comes first, then command-line packs. Duplicate IDs are loaded only once, at their first occurrence. Later packs override earlier packs. The command-line `--list-mods` option lists active packs. Changes take effect after restart; hot reloading is not supported.

## Generation settings

`world.txt` is optional. Omitted fields keep the previous pack's value or the default.

| Key | Default | Allowed range | Effect |
| --- | --- | --- | --- |
| `tree_density` | 0.62 | 0–1 | Chance of a tree at each eligible 8-unit lattice site |
| `lake_threshold` | 0.28 | 0–0.6 | Larger values produce more frontier water |
| `biome_scale` | 192 | 64–1024 | Size of smooth climate/lake features |
| `enemies_per_chunk` | 3 | Integer 0–8 | Candidate enemy spawns per 64×64 chunk |
| `respawn_seconds` | 45 | 5–600 | Delay before defeated enemies replenish |
| `level_distance` | 120 | 30–10000 | Distance per additional enemy level; caps at level 100 |
| `seed_salt` | 0 | Integer 0–4294967295 | Mixed with both halves of the selected world seed |

For example:

```text
tree_density 0.8
lake_threshold 0.35
biome_scale 240
enemies_per_chunk 5
respawn_seconds 25
```

Candidates in water, inside obstacles or too near the player are skipped. Spawn settings therefore specify candidates, not guaranteed counts. The active mob limit remains 100; the chunk cache remains 49 chunks. Mods cannot disable these performance bounds. Generation tweaks preserve the authored starting lake, ruins and landmark clearings; lakes/biomes beyond the starting area use the procedural rules.

## Content tables

A pack may contain `classes.txt`, `abilities.txt`, `enemies.txt`, `items.txt`, `boss.txt` and `boss_phases.txt`. Each supplied file replaces the entire corresponding table. The last pack supplying a filename wins. A missing file falls back through earlier packs to the base `data/` directory (or `--data DIR`). Copy a base file before editing it; headers describe the columns.

Keep all five classes and six abilities per class. Retain the first three enemy archetypes and first five item IDs: the existing skeleton/wisp/sentinel and Dragon rewards refer to them. Append new enemies as extra rows; they automatically join procedural spawning. Item IDs must remain contiguous from zero, and every loot reference must exist. The engine currently accepts up to 128 enemy definitions and 1,024 items. Boss phases must contain exactly phases 1, 2 and 3.

Names must be printable ASCII, nonempty and at most 60 characters. Numeric values are validated; malformed files, unsupported API versions, unknown generation fields, unknown ability effects and unsupported magnitudes stop startup with an error instead of silently running a partially loaded pack. Existing ability effects can be retuned but new effect names require C++ implementation in `src/combat/combat.cpp`.

See `mods/wild_frontier/` for a working pack: it replaces the enemy table, preserving the base rows and adding a Frontier Stalker.

## Determinism and current limits

The generator combines the world seed, mod seed salt and world coordinates. Terrain is reproducible for the same build, seed and effective mod configuration regardless of visitation order. Changing any of those can change the layout. Adventure saves store generation settings and require matching enabled mod names and item tables. Keep the same pack versions for a continued world; arbitrary changes to abilities/enemies are not migrated.

Only nearby chunks and encounters are retained. Terrain is regenerated on return; enemy kill timers are remembered until their respawn deadline, but surviving distant enemies return with fresh state. Distant dropped loot is discarded. The five named origin landmarks remain fast-travel anchors; the local map and testing-mode `T` jump help inspect the generated frontier.

The algorithm covers a very large area, but the float-position engine deliberately bounds travel at ±1,000,000 world units to avoid unstable movement. This is streamed procedural exploration, not literal infinity, a voxel sandbox, or a persistent terrain-editing system.

C++ extension points: `GenerationConfig` and `loadMods()` in `src/mods/`, `World::terrainNoise`, `treeAt`, `streamChunks`, `biome` and `water` in `src/world/world.cpp`, and `Game::streamEnemies` in `src/game.cpp`.
