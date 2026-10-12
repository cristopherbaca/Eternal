# 3D renderer

The initial foundation was Step 1. The native window now draws a triangle terrain mesh through a perspective camera into RGBA16Float color/normal targets and a Depth32Float target at the ASCII grid resolution (now 4×8 window points per cell at the default Detailed setting). A GPU ASCII conversion pass presents those cells through the existing glyph atlas; see the density correction below for the current implementation. F8 exposes the raw color target. The source frame remains GPU resident; no per-frame pixel readback is used.

The new camera module supplies both CPU world/screen projection and the Metal camera basis. Picking marches a screen ray to the nearest terrain crossing, refines it, and uses the same triangle diagonal as the mesh. World objects and player labels use terrain heights through this module. WASD follows the orbit camera. A bounded 7×7 mesh cache regenerates only entering chunks and uploads when its revision changes. The terrain generator, seed and save format are unchanged.

Controls: right mouse drag orbits/tilts (45–65 degrees); wheel or existing zoom keys change distance; [ and ] adjust FOV; F8 switches raw/ASCII; F9 shows GPU pass timings and chunk count. Run `./build/eternal_rpg --window`. `--raw-3d` starts in raw view; `--renderer-2d` retains the previous native renderer. Terminal and interiors currently retain their existing renderer.

HUD and menus keep their layout and the original crisp atlas overlay, including dimming behind menus. No combat/class/save implementation was changed. Existing character, monster, prop and spell glyphs remain transitional projected overlays; low-poly depth-tested models and particles belong to Step 4. They do not yet participate in the terrain depth buffer.

## Verification

247 core checks pass, including nine new camera/mesh/input checks; the pre-existing suite still covers combat, spells and saves. Native checks verify offscreen terrain completion, F8/F9 routing, perspective mouse casting, focus, held movement and menus.

A 240-frame native run at 160×50 source cells measured 59.77 submissions/s on the synchronized window, 6.00 ms CPU mean / 7.48 ms p95, 0.313 ms mean terrain GPU time and 2.357 ms mean ASCII/atlas/HUD composite GPU time. GPU values are command-buffer timestamps for the two separately submitted passes. They are not display scanout measurements.

## Captures and reference comparison

- [Raw 3D](screenshots/18-3d-foundation-raw.png)
- [Current ASCII output](screenshots/18-3d-foundation-ascii.png)
- [Existing forest/reference-era screenshot](screenshots/10-forest.png)
- [Same desert location before conversion](screenshots/17-terrain-desert.png)

Raw and ASCII captures use seed 42, `--no-mods --position -984 -3000 --demo`. The old supplied video paths are absent and ffmpeg is unavailable; no matching extracted video frame could be inspected. The reference links are repository screenshots, not substituted video frames.

The foundation resolves the key geometric gap: perspective triangles, real terrain depth occlusion, an orbit camera and matching terrain picking. The current ramp still produces broad uniform glyph bands. Next, Step 2 will add depth/normal edges, finer slope readability, world-stable detail and temporal stabilization. Reference vegetation volume, soft shadows, warm glow, reflective water and reactive grass remain Steps 3–5. Camera collision and final fallback polish remain Step 6. Wait for the user's “next” before Step 2.

## First-person correction

The native 3D view now defaults to a player-eye camera, 1.75 world units above the terrain. Yaw and pitch rotate in place; camera position follows the player directly. Mouse look is captured while the game window is active in gameplay, and released for menus, loss of focus and exit. WASD is camera-relative, the center crosshair controls terrain aiming, and the local player's sprite/name are hidden. Existing planar combat remains unchanged. `--third-person` selects the previous orbit view. F8 still toggles raw/ASCII, F9 diagnostics, and the wheel changes view magnification. Distant glyph objects are clipped to the terrain far plane.

253 core checks pass, including eye position, crosshair projection, rotation in place and far clipping. The native app successfully rendered both [raw](screenshots/19-first-person-raw.png) and [ASCII](screenshots/19-first-person-ascii.png) captures. Full native input automation did not pass in this run because macOS did not consistently give the test window keyboard focus (including the pre-existing held-WD check); active mouse capture still needs a focused interactive desktop check.

This corrects the intended viewpoint. The broad brightness bands and temporary glyph models remain the next visual work; they are not presented as finished first-person graphics. Step 2 is still waiting for the user's next instruction.

## ASCII density and readability correction

The first-person path accidentally used the coarse HUD grid for the world. It now uses `Window::size(detail)` independently: Comfort ~213×66, Detailed 320×100, Dense ~426×133 at a 1280×800 window. HUD text remains 160×50. Choose **Settings → Window detail**; F9 shows the actual source grid. On a 2× Retina display, Detailed corresponds to 8×16 physical pixels per cell.

A compute kernel converts each source cell once, using neighboring depth/normal discontinuities for contour glyphs, a brightness ramp, smooth world-position detail fading with distance, brighter foreground colors and subtle source-colored cell backgrounds. This preserves terrain color and silhouettes between glyph strokes. Detail does not change randomly with frame time. There is no temporal history/reprojection yet; stability during interactive movement still needs review. The existing glyph entities and separate crisp HUD are composed afterwards. Raw F8 preview bypasses glyph presentation of terrain.

Build and 253 core regression checks pass. `MTL_DEBUG_LAYER=1 ./build/eternal_native_input_tests --renderer-only` passes all density presets, GPU buffer reallocations, independent HUD dimensions, raw preview and 2D fallback without relying on macOS input focus. This is renderer validation, not a replacement for the outstanding focused desktop input check.

A 240-frame Detailed ASCII run measured 58.70 submissions/s at a 60 target, CPU work 3.92 ms mean / 5.65 ms p95, terrain GPU 0.417 ms and conversion + atlas/HUD composite GPU 2.120 ms. Conversion and composition share one command buffer and are reported together. No capture or Metal validation was enabled during this timing run.

- [Revised ASCII](screenshots/20-density-ascii.png)
- [Matching raw view](screenshots/20-density-raw.png)
- [Previous first-person density](screenshots/19-first-person-ascii.png)

Same seed/location as the earlier captures. The revised image has finer horizon contours and stronger continuous terrain colors. Flat placeholder creatures, terrain glyph bands and missing 3D vegetation remain visible gaps; density alone cannot supply the later object-model stage. The original reference videos are still unavailable, so these comparisons use existing game captures, not extracted reference frames. This is a scoped density correction, not completion of all Step 2 temporal/debug features or later renderer stages.

## Creature occlusion, distance scaling and sprint

World glyph cells now carry Metal device depth. The composite shader compares them against the offscreen terrain depth, so hills conceal creatures and their labels in both ASCII and raw views. Scene overlap uses camera distance rather than world Y. HUD text and the aiming crosshair remain independent overlays. This does not turn the existing glyph sprites into polygon models or make them cast shadows.

Humanoids/skeletons use a 2.4-unit height, wolves 1.2 units, and the Dragon retains its larger silhouette. Perspective size follows distance, FOV and source-grid resolution. Close-up loops are clipped to the viewport. World labels track the creature height rather than a fixed screen offset.

Hold **Shift + WASD** in the native window to sprint at 1.8× walking speed. Releasing Shift restores walking speed. Diagonal movement stays normalized; existing slopes, collision, water and slow penalties still apply. Menus and inactive windows disable sprint. Combat/classes, HUD layout, save schema and the seed system are unchanged.

Validation: 260 core checks pass; GPU image comparisons under Metal API validation verify hidden and visible glyphs, plus density resize, raw preview and fallback. Hardware Shift/focus behavior was not verified interactively. Final native 240-frame result: 58.62 submissions/s at 60 target, 7.06 ms mean / 10.58 ms p95 CPU work, 0.287 ms terrain GPU and 2.331 ms conversion/composite GPU.

[ASCII capture](screenshots/21-creature-depth-ascii.png) · [Matching raw view](screenshots/21-creature-depth-raw.png). The previous screenshots above show the same seed/location before depth rejection. Reference videos remain unavailable at the supplied paths.
