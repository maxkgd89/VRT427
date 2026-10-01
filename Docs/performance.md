# VRT performance and Quest checklist (plan step 9)

Targets: PC VR (Rift S) 80 Hz without dropped frames; Quest standalone later.
Run every check in a packaged-like situation: **VR Preview**, `L_Test4`, `VRT.Debug.*` all 0.

## 1. PC baseline (Rift S)
1. Open the console in the editor (`~`) during VR Preview.
2. `stat unit` — Frame, Game, Draw and GPU should all stay below 12.5 ms (80 Hz). Note the numbers.
3. `stat fps` — should hold 80.
4. `stat gpu` — note the biggest passes. Fog, sky atmosphere and sky light are the expected costs in the test level.
5. `stat scenerendering` — note the draw calls and the mesh draw calls. The labyrinth is one static mesh actor per wall segment plus the floor, so this number grows with the maze; keys, beacons and the exit add a few.
6. Fire the gun at full auto against a wall for 10 seconds, then run `stat unit` again. Projectiles are short-lived actors, so watch for Game-thread spikes.
7. Write the numbers into the table below.

| Scenario | Frame ms | Game ms | Draw ms | GPU ms | Draw calls |
|---|---|---|---|---|---|
| Standing at spawn | | | | | |
| Walking in the labyrinth | | | | | |
| Full-auto fire at a wall | | | | | |
| Comfort vignette on (`bComfortVignette`) | | | | | |

## 2. Cheap things already done
- Debug drawing only happens behind the `VRT.Debug.*` cvars. In **Shipping** builds the debug-only ticks (hands, holsters, trigger spheres) are not even registered, and `DrawDebug*` calls compile out.
- Per-frame work that remains: the pawn (movement, body anchor, wrist text), key spin, and the gun while two-handed.
- No physics simulation on weapons; projectiles are swept spheres with no gravity.
- Beacons and the exit pad are unlit, no shadows, no collision.

## 3. Things to try if a number is too high
- Sky: replace `SkyAtmosphere` + movable sun/sky light with a static sky and fixed lighting (`r.AllowStaticLighting` is on).
- `r.ScreenPercentage` / Oculus pixel density.
- Turn off `bComfortVignette` (it adds a post-process pass; vignette is not supported by the Quest mobile renderer without Mobile HDR).
- Fog: remove `ExponentialHeightFog` on Quest.

## 4. Quest standalone sanity check (Quest 2 / 3)
Project settings already have Vulkan, Mobile Multi-View (`vr.MobileMultiView=True`) and `PackageForOculusMobile` for Quest/Quest2. To check:
1. Project Settings → Platforms → Android: package for Oculus Mobile devices, ARM64, Vulkan, Multi-View on.
2. File → Package Project → Android (ASTC). Install with `adb install`.
3. Check it starts, the headset tracks, the grab/holster/shoot loop works and the frame rate holds (`adb shell setprop debug.oculus.gpuLevel` / OVR Metrics Tool).
4. Things known to differ on mobile: unlit emissive materials are fine; translucent materials and post-process are expensive; the wrist text uses the engine's default font material.

## 5. Release tag
After the headset tests pass and the numbers are written down, commit and tag **`mechanics-complete`** (GitHub Desktop: right-click the commit in History → Create Tag).
