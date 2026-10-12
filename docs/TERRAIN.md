# Terrain generation and verification

Terrain uses the existing 12-digit world seed and mod generation settings. Four noise octaves, domain warping, continental basins, ridges and plateau shelves define elevation. Heights and climate samples share a global four-unit lattice; the 49 nearby chunks stream lazily and regenerate identically. The sampling cache is bounded and copy-safe for save loading.

The camera applies a small height offset to terrain, trees, world objects, labels and mouse projection. Fixed sun lighting uses terrain normals; steep slopes transition to rocky slash/hash glyphs. Walking slows uphill and cliffs block movement. Roads, settlement clearings and established landmark approaches remain accessible. Collision subdivides long movement steps.

Eight biome weights derive from height, moisture and temperature. Palettes blend continuously, and coordinate-seeded glyph mixing avoids hard texture borders. Biomes control grass density, flowers/reeds/cacti, broadleaf/pine/swamp trees, boulders and at most 48 ambient motes. Existing grass bending and water reflections remain. Water includes the reference lake, height-derived frontier basins and warped contour river channels, with brighter animated blue-green surfaces. These are procedural channels, not a drainage/erosion simulation. Boulders are visual terrain detail.

Region names update through the existing footer. Ashenmere Ruins, Frostwood, Ember Hills, Reedlands and Dragon Hollow retain their names near the original landmarks. No combat, class, HUD/menu layout, NPC or quest logic was edited. The only game integration is grounding the existing render camera in World.

Existing saves still load; their regenerated terrain now follows this improved algorithm, so previous terrain shapes are not frozen. No new save fields are required.

## Reproducible biome locations

Use seed 42, no mods, and `--position X Y`:

| Biome | X | Y |
| --- | ---: | ---: |
| Meadow / Roadlands | -2584 | -3000 |
| Deep forest | -3000 | -3000 |
| Pine / frost forest | -2648 | -3000 |
| Swamp | -2872 | -3000 |
| Desert / dunes | -984 | -3000 |
| Rocky highlands | -2712 | -3000 |
| Snowy mountain | -2040 | -3000 |
| Beach / coast | -2936 | -3000 |
| River through frost forest | -2632 | -3000 |

Example: `./build/eternal_rpg --window --demo --no-mods --seed 42 --position -984 -3000`

## Checks

238 core checks pass, including terrain seed variance, cached/uncached agreement, seam continuity, slope movement/cliffs, camera inverse projection, regeneration, all eight biomes, normalized continuous biome blends and vegetation regeneration. Native Metal input tests pass.

At 320×100, 120-frame headless frontier run: mean 10.78 ms, p95 12.19 ms. Native 240-frame dune run at a 60 FPS target: 58.90 submissions/s, mean 13.34 ms, p95 13.34 ms. This stays within the 16.67 ms work budget on this test machine; submission rate is not verified display scanout. Performance depends on resolution and scene.

Native captures: [dunes](screenshots/17-terrain-desert.png), [snowfields](screenshots/17-terrain-snow.png), [coast](screenshots/17-terrain-coast.png), [river](screenshots/17-terrain-water.png). Earlier video attachment paths were unavailable, so repository screenshots were used as visual references.
