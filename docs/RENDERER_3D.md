# 3D renderer — Step 1 foundation

Completed this step only. The native window now draws a triangle terrain mesh through a perspective camera into RGBA16Float color/normal targets and a Depth32Float target at the ASCII grid resolution (about 8×16 window points per cell). A preliminary brightness ramp presents those cells through the existing glyph atlas. F8 exposes the raw color target. The source frame remains GPU resident; no per-frame pixel readback is used.

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
