# VRT — Development Plan

UE 4.27 · C++ first · OpenXR · PC VR (Rift S / Quest 3 via Link) first, Quest standalone later.
Project: `E:\UE_projects\VRT` · Engine: `E:\UE_4.27`

---

## 1. Game concept (summary)

- Player spawns inside a labyrinth with 2 m walls.
- 4 keys are hidden in the labyrinth. Each key has a tall blue **beacon** column (10–20 m) beside it, visible above the walls.
- Keys are picked up by walking through an invisible trigger volume.
- The player returns to the spawn point; the **exit trigger** completes the level if the player has **≥ 2 keys**.
- Two weapons, both grabbed with the **right hand** from body holsters:
  - **Pistol**: one-handed, slow fire rate, holstered at the right waist. Shape: small long box.
  - **Gun**: two-handed, fast fire rate, holstered over the right shoulder. Twice the pistol's length. The left hand grabs only the barrel and helps aim.
- Weapons have no physics. When released they disappear and respawn in their holster.
- Unlimited ammo for now.
- Movement is horizontal only (no jumping). The pawn has gravity only so the capsule settles back to the floor after stepping over low obstacles. Walls block the pawn's capsule.

Later: enemy AI, spawning director, combat, health, ammo, procedural labyrinth.

**Flat playtest mode (VRF, step 11.5):** a second, top-down flat-screen game mode used only for developing AI and levels (debug spheres, EQS, enemy routes are easier to follow on a monitor than in the headset). The final game is VR only.

---

## 2. Target architecture

### Source layout
```
Source/VRT/
  Player/      VRTPlayerPawnBase, VRTPawn, VRTHandComponent, VRTHolsterComponent,
               VRFPawn, VRFPlayerController                             (VRF: step 11.5)
  Weapons/     VRTWeaponBase, VRTPistol, VRTRifle, VRTProjectile, VRTShot (shared fire helper)
  Gameplay/    VRTGameMode, VRFGameMode, VRTGameState, VRTKey, VRTBeacon, VRTLevelExit, VRTTriggerBase
  Level/       VRTMazeGenerator, VRTMazeData, VRTMazeBuilder        (step 10)
  AI/          VRTEnemy (+ V0/V1/V2), VRTEnemyController, VRTNoiseSubsystem, VRTDirector   (steps 13–14)
  Combat/      VRTHealthComponent, VRTDamageTypes                   (step 11+)
  UI/          VRTWristDisplay (step 8–9), VRFHUD (step 11.5)
  Debug/       VRTDebugSettings (console vars)
```

### Key classes
| Class | Base | Role |
|---|---|---|
| `AVRTPlayerPawnBase` | APawn | Step 11.5. Shared parent of both player pawns: capsule (`VRTPlayer` profile), `UFloatingPawnMovement`, `UVRTHealthComponent`, walk/run speeds, death → `RestartCurrentLevel()`. Virtual `PlayHapticPulseBothHands()` (empty in the base), `NotifyTeleported()`, `GetPawnViewLocation()` (the eye point enemies trace to). Gameplay code outside `Player/` uses this class, never `AVRTPawn`. |
| `AVRTPawn` | AVRTPlayerPawnBase | VR pawn: HMD, controllers, smooth move, snap turn, hands, holsters, wrist display. `GetPawnViewLocation()` = HMD camera. |
| `AVRFPawn` | AVRTPlayerPawnBase | Step 11.5. Flat top-down test pawn: WASD, mouse aim, built-in full-auto fire with the rifle's values, scroll zoom, free camera. `GetPawnViewLocation()` = floor + `EyeHeight`, never the camera. |
| `AVRFPlayerController` | APlayerController | Step 11.5. Mouse cursor visible, game-and-UI input mode, fog off. |
| `AVRFHUD` | AHUD | Step 11.5. Text HUD (HP, keys, exit, level, seed, zoom, free-cam), red damage flash. |
| `UVRTHandComponent` | USceneComponent | One per hand. Grip/trigger state, overlap sphere for grabbing, current held item, haptics. |
| `UVRTHolsterComponent` | USceneComponent | Body-relative zone (waist or shoulder) with a grab radius. Holds and respawns its weapon. |
| `AVRTWeaponBase` | AActor | No physics. Attach/detach to hand, fire rate, muzzle, projectile class, return-to-holster. |
| `AVRTPistol` / `AVRTRifle` | AVRTWeaponBase | One-handed and two-handed variants (the rifle adds a barrel grip point). |
| `AVRTProjectile` | AActor | Sphere collision plus UProjectileMovementComponent, no gravity, lifetime, damage (later). |
| `AVRTTriggerBase` | AActor | Sphere trigger with an optional debug-visible sphere mesh. Overlap filtered to the player pawn. |
| `AVRTKey` | AVRTTriggerBase | Gives a key on overlap, then hides and switches off its beacon. |
| `AVRTBeacon` | AActor | Tall blue unlit emissive column, no collision, height parameter. |
| `AVRTLevelExit` | AVRTTriggerBase | At spawn. Checks key count ≥ `RequiredKeys` (2) and completes the level. |
| `AVRTGameState` | AGameStateBase | Keys collected and keys total, level index, seed, level state. |
| `AVRTGameMode` | AGameModeBase | Exists already. Level flow: Start → Playing → Complete → next level. |
| `AVRFGameMode` | AVRTGameMode | Step 11.5. Same level flow; only swaps pawn, controller and HUD classes. Chosen per map in World Settings. |
| `AVRTEnemy` | APawn | Step 13. Capsule, static mesh, `UFloatingPawnMovement`, health. Subclasses `AVRTEnemyV0/V1/V2` set feature flags and tunables. |
| `AVRTEnemyController` | AAIController | Step 13. Pure C++ state machine, sight checks, spawn mode (roaming / sleeping). |
| `UVRTNoiseSubsystem` | UWorldSubsystem | Step 13. Weapons report gunshots; sleeping enemies and V1 listen. |

### Engine modules to add to `VRT.Build.cs` (as needed)
`UMG`, `Slate`, `SlateCore` (wrist UI) · `AIModule`, `NavigationSystem`, `GameplayTasks` (AI, steps 11+).

### Collision setup (DefaultEngine.ini)
- New object channel **Projectile**. New trace channel **Grab** (optional; overlaps are enough).
- Profiles:
  - `VRTPlayer` (capsule): blocks WorldStatic, overlaps Trigger.
  - `VRTTrigger`: overlaps Pawn only.
  - `VRTProjectile`: blocks WorldStatic and Pawn/Enemy, ignores the player and the weapons.
  - `VRTWeapon`: no collision while held.

### Input
Move from hard-coded `EKeys::OculusTouch_*` bindings to named Action/Axis mappings in DefaultInput.ini. This lets Index, WMR and Quest controllers work, and lets the fire button be remapped:
- `GrabLeft` / `GrabRight` (grip), `FireLeft` / `FireRight` (index trigger), `MoveX` / `MoveY`, `Turn`, `ToggleRun` (left Y button switches walk/run mode), `RecenterHMD` (left X button).
- **VRF (flat mode, step 11.5)** uses its own names with the `VRF_` prefix, so keyboard/mouse never leak into VR bindings:
  - axes `VRF_MoveForward` (W/S), `VRF_MoveRight` (D/A), `VRF_Zoom` (MouseWheelAxis), `VRF_MouseX` / `VRF_MouseY`;
  - actions `VRF_Fire` (LMB), `VRF_Run` (LeftShift, hold), `VRF_FreeCamera` (F), `VRF_PanDrag` (MiddleMouseButton).

---

## 3. Steps 1–9: mechanics on a hand-built test level

Each step ends with a playable test in the headset.

### Step 1 — Cleanup and foundation
- Delete `MyClass.h/.cpp`. Create the source folders above.
- Add `VRTLog.h/.cpp` with all log categories (see `AGENTS.md` → Logging) and route existing pawn code through `LogVRTPawn` / `LogVRTInput`.
- Check VRTemplateMap's World Settings: no GameMode override to the template's `VRGameMode`, and no placed BP `VRPawn`.
- Switch pawn input to named mappings (see Input above).
- Add collision channels and profiles.
- Make capsule size changes in the editor take effect (apply size in `OnConstruction`).
- Add `VRT.Debug.*` console variables (show triggers, show holster zones, draw aim lines).
- **Test:** existing move and snap turn still work through the named mappings.

### Step 2 — Test level `/Game/VRT/Maps/L_Test`
- New persistent map with a simple floor and a hand-made small labyrinth of 2 m walls (StarterContent walls or `1M_Cube` scaled).
- Corridors **3–4 m** wide, so a two-handed gun doesn't hit walls constantly.
- Lighting: one movable directional light, sky light, fog. No baked lighting (the level will be procedural later).
- Spawn point (PlayerStart) marked.
- **Test:** walk around; capsule collision against walls; check comfort at walk and run speeds.

### Step 3 — Hands: grab detection
- `UVRTHandComponent` on each motion controller: overlap sphere (≈ 8 cm), grip pressed/released events, held-item pointer, haptic helper.
- Grab rule: on grip press, find the best candidate in range (holster zone or barrel grip point). On grip release, drop the held item.
- Debug: hand sphere changes colour when a grab target is in range.
- **Test:** grip near a debug target highlights and logs.

### Step 4 — Holsters (body zones)
- `UVRTHolsterComponent` is attached to a **body anchor**. The body anchor follows the HMD position, takes yaw only (pitch and roll ignored), and sits at a fixed offset below the head. The holsters stay on the "body" when the player looks around.
- Zones:
  - **Waist-right**: about 0.55 × head height, 20 cm to the right, slightly forward. Holds the pistol.
  - **Shoulder-right**: about 10 cm below head height, 15 cm right, 10 cm behind the head. Holds the gun.
- Grab radius ≈ 15–20 cm. Only the **right** hand can draw from holsters.
- The holstered weapon is visible in its zone (optional per-zone toggle). A haptic pulse fires when the hand enters a zone.
- **Test:** reach to the waist and to the shoulder; the zones respond; they follow body turning (snap turn and real turning).

### Step 5 — Weapon base and pistol
- `AVRTWeaponBase`: no physics simulation, collision off while held. `AttachToComponent(hand, SnapToTarget)` at a grip offset.
- On release: detach → hide → teleport back to the holster → show (optional short fade/scale-in). The weapon is never destroyed, so no garbage-collection churn.
- `AVRTProjectile`: small sphere, `ProjectileMovement` at about 3000–6000 cm/s, no gravity, 3 s lifetime, destroyed on hit with a debug hit effect. Use **pooling** later if needed.
- **Pistol**: box about 4 × 3 × 20 cm. Fire interval about 0.4 s (slow). Semi-auto: one shot per trigger press. Muzzle at the box front.
- Fire button: **right index trigger** (the hand holding the weapon). Kept as a single `FireAction` property so it can be remapped.
- Haptic pulse on fire. Basic sound (`Fire_Cue` from the template).
- **Test:** draw the pistol from the waist, shoot walls, release it and see it return to the waist.

### Step 6 — Two-handed gun
- **Gun**: box about 4 × 4 × 40 cm (twice the pistol's length). Fire interval about 0.1 s (fast), full-auto while the trigger is held.
- Right hand grabs from the shoulder zone and becomes the primary grip.
- **Barrel grip point**: a zone on the front half of the gun. Only the **left** hand can grab it, and only while the right hand holds the gun.
- Aiming:
  - Two-handed: gun forward = (left hand − right hand) normalized; up = right controller up projected onto that axis. Position stays at the right grip.
  - One-handed (left released): the gun follows the right controller.
- Releasing the right hand also frees the left, and the gun returns to the shoulder.
- Logging: `LogVRTTwoHand` logs every state change (Log) and per-frame aim math (VeryVerbose, throttled); `VRT.Debug.TwoHand 1` draws grips, aim axis and resulting gun axes. See `AGENTS.md`.
- Holding rules: the right hand holds one weapon at a time; drawing is blocked while a weapon is held.
- **Test:** draw from the shoulder, grab the barrel, aim along the two-hand line, spray fire, release.

### Step 7 — Keys and beacons
- `AVRTTriggerBase`: `USphereComponent` trigger (radius ≈ 75–100 cm) plus a `bShowDebugSphere` mesh (translucent sphere, editor and runtime toggle via cvar).
- `AVRTKey`: on player overlap → `GameState->AddKey()`, haptic and sound, hides itself and tells its beacon to fade out.
- `AVRTBeacon`: cylinder about 0.5 m wide, **15 m tall** by default (editable 10–20 m). Unlit emissive blue material, no collision, no shadow casting. Optional slow pulse. Placed next to its key (keys own a beacon reference, or spawn it).
- Placed in L_Test: 4 keys at the far parts of the labyrinth.
- **Test:** beacons are visible above the walls from anywhere; walking through a key collects it and switches its beacon off.

### Step 8 — Level exit and game flow
- `AVRTLevelExit` at the spawn point, sphere trigger with a debug sphere.
- `RequiredKeys = 2`, `TotalKeys = 4`. Collecting more than 2 is optional; extra keys can later give a bonus score.
- On overlap:
  - keys ≥ 2 → `GameMode->CompleteLevel()`
  - otherwise feedback ("need N more keys", haptic buzz).
- The exit is visually inactive until 2 keys are collected (colour change on the debug sphere or a small beacon).
- `AVRTGameMode` flow: **Start → Playing → Complete → (fade) → next level**.
  - On the test level, "next level" restarts the same map.
  - From step 10 on, it regenerates the labyrinth with a new seed.
- Use camera fade (`PlayerCameraManager->StartCameraFade`) for transitions. This avoids the hard cut and loading hitch that `OpenLevel` causes in VR.
- **Wrist display** (left wrist; implemented as a `UTextRenderComponent` that faces the head, so no UMG/widget assets are needed; can be swapped for a `UWidgetComponent` later): keys X/4, the exit requirement, level number.
- **Test:** the full loop of collect 2+ keys → return → level complete → restart.

### Step 9 — Integration, debug and performance pass
- All debug visuals behind cvars (`VRT.Debug.Triggers 0/1`, etc.). Shipping defaults: triggers invisible.
- Tuning pass: holster offsets, grip offsets, fire rates, projectile speed, walk/run speed, snap angle.
- Comfort: optional vignette while moving (post-process material, if affordable on Quest).
- Performance baseline on Rift S (90 fps / 80 Hz target): `stat unit`, `stat gpu`, draw-call count.
- Quick Quest standalone build sanity check (packaging works, Multi-View on, frame rate OK).
- Tag a git release: **"mechanics-complete"**.

---

## 4. Step 10 — Procedural labyrinth

### 10.1 Data model
- Grid of `W × H` cells. Each cell stores 4 wall bits (N/E/S/W) and flags (spawn, key, dead-end, room, region id).
- Pure C++ data (`FVRTMazeData`), not tied to actors. Easy to unit test, to debug-draw and to reuse for AI (flow distance).
- Deterministic from a seed (`FRandomStream`), so any level can be reproduced from its seed (shown on the wrist UI for bug reports).

### 10.2 Generation algorithms (compared)
| Algorithm | Character | Fit for this game |
|---|---|---|
| **Recursive backtracker (DFS)** | Long winding corridors, few branches, few dead ends | Good for exploration; can feel like one long tube. Fine as the default. |
| **Randomized Prim's** | Many short branches and dead ends, "bushy" | More hiding places for keys; less flow. |
| **Kruskal's** | Uniform-ish, many short dead ends | Similar to Prim's; easy to add "pre-connected" rooms. |
| **Wilson's / Aldous-Broder** | Unbiased (uniform spanning tree) | Statistically "fair"; slower; no strong advantage here. |
| **Growing Tree** | Parameter mixes DFS and Prim's (newest vs random cell) | **Recommended**: one algorithm, tunable per level (e.g. 75 % newest / 25 % random). |
| **Eller's** | Row-by-row, constant memory, can be endless | Only useful for infinite or streamed mazes. Keep as an option. |
| **Binary tree / Sidewinder** | Strong diagonal bias, trivial | Too predictable; skip. |

**Post-processing (important for a shooter):**
1. **Braiding**: remove 10–30 % of dead ends by knocking out a wall. This creates loops, so enemies can flank and the player can escape (L4D-like). Pure perfect mazes are bad for combat.
2. **Rooms / arenas**: carve 1–4 open areas (2×2 to 4×4 cells) before or after generation. Use them for fights, key locations and the spawn hub.
3. **Spawn hub**: a 2×2 or 3×3 open area at the spawn point.

### 10.3 Key placement
- BFS from the spawn cell gives each cell a path distance.
- Divide the maze into 4 quadrants (or 4 regions by distance). In each region pick a cell from the top 20–30 % of distance, preferring dead ends or rooms.
- Enforce a minimum path distance between keys. The beacon spawns in the same cell, offset to a corner.
- The same distance field later drives the **AI director** (spawn enemies ahead of or behind the player along the flow distance, as L4D does).

### 10.4 Building geometry
- Cell size **4 m** (3.8 m clear corridor plus 0.2 m walls). Wall height 2 m.
- Meshes: one wall segment (4 m × 0.2 m × 2 m) plus a corner pillar (0.2 × 0.2 × 2 m). Optionally 2–3 variants for visual variety.
- **Hierarchical Instanced Static Mesh (HISM)** per mesh type: thousands of walls in a few draw calls, critical for Quest.
- Floor: one large plane or tiled instances; ceiling none (beacons must be visible).
- Collision: simple box collision on the wall mesh. The HISM handles it.
- Lighting: fully dynamic (movable directional plus sky light) or mostly unlit/fog style. Shadows off on Quest. Emissive beacons look good in a darker, foggy style and cost little.

### 10.5 Size suggestions
Walk speed 1.5 m/s, run 3.5 m/s.
| Level | Grid | World size | Approx. walls | Notes |
|---|---|---|---|---|
| 1 | 8 × 8 | 32 × 32 m | ~150 | Tutorial |
| 2–3 | 12 × 12 | 48 × 48 m | ~300 | |
| 4–6 | 16 × 16 | 64 × 64 m | ~550 | Enemies start |
| 7+ | 20 × 20 – 24 × 24 | 80–96 m | ~850–1200 | |
| Max | 32 × 32 | 128 × 128 m | ~2100 | Upper bound for Quest with AI |

Grow the size per level; also scale braiding, number of rooms and enemy budget.

*Implemented in `Source/VRT/Level/VRTLevelProgression.h`:* levels 1 / 2-3 / 4-6 / 7-9 / 10-12 / 13-15 / 16+ use 8 / 12 / 16 / 20 / 24 / 28 / 32 cells per side. Rooms grow 2 to 6, braiding 15% to 30%, the Growing Tree bias drops from 0.75 to 0.55 (bushier). Level N uses seed + N - 1. Levels are built in place behind a fade (`AVRTGameMode::GoToLevel`); maps without a maze builder still reload with `?Level=N`.

### 10.6 Streaming and level division
- **Level streaming (sublevels / World Composition) is not needed.** It is designed for hand-authored content. A procedural maze up to about 130 m with HISM walls fits easily in one level in memory (a few thousand instances).
- **Chunking is worth doing.** Split the maze into chunks of 8 × 8 cells (32 m), each with its own HISM component. This gives:
  - frustum and distance culling per chunk (one HISM over the whole maze culls less well);
  - cheap partial rebuilds (e.g. opening a secret wall);
  - natural "regions" for the AI director (active vs dormant chunks).
- **Actor "streaming" for enemies and pickups**: pool them and activate only within N chunks of the player. Tick off, hidden, AI paused when far. This is where the real performance budget goes.
- **Map structure:** one persistent map `L_Maze`, holding lighting, post process, nav bounds and the generator actor. Level transitions regenerate in place behind a fade (no `OpenLevel`).
  - Optional: sublevels only for hand-made set pieces (boss room, hub) streamed in and placed at a carved room.
- **Navigation:**
  - NavMesh `RuntimeGeneration = Dynamic`, with a NavMeshBoundsVolume sized to the max maze. Rebuild after generation, either for the whole maze or tile-by-tile around the player (navigation invokers) on large maps.
  - The grid itself serves as a cheap high-level graph for the director (distances, choosing spawn cells).

### 10.7 Step 10 sub-tasks
1. `FVRTMazeData` plus a Growing Tree generator plus braiding plus rooms, seeded. Debug draw.
2. `AVRTMazeBuilder`: chunked HISM walls, floor, spawn hub, PlayerStart placement.
3. Key and beacon placement via BFS. Exit at spawn.
4. GameMode: regenerate on "next level" with seed+1 and increased size.
5. Runtime NavMesh rebuild (prepares for AI).
6. Performance check on the max size (Rift S, then Quest).

---

## 5. Steps 11+ — Combat and AI (outline)

### Step 11 — Health and damage
- `UVRTHealthComponent` (health, max, `OnDamaged`, `OnDeath`) used by the player and enemies. Uses the UE `TakeDamage` / `ApplyPointDamage` flow.
- Projectiles deal damage. Hit feedback: haptics and a screen-edge red vignette for the player.
- Player death → fade → restart the level (same seed).
- From step 11.5: health, damage feedback hooks and death handling live on `AVRTPlayerPawnBase`, shared by the VR and the flat pawn.

### Step 11.5 — VRF flat playtest mode (AI and level testing)
Goal: play the same maps on a monitor from a top-down camera, to see debug spheres, EQS, enemy routes and AI behaviour clearly. VR stays the real game; VRF is a dev tool.

Rules:
- Same maps, keys, exit, damage and level flow as VR. Only the pawn, controller and HUD differ.
- **Selection:** World Settings → *GameMode Override* = `VRFGameMode` on the map being tested. Play with *Selected Viewport* or *New Editor Window*, not VR Preview. Clear the override to go back to VR.
- VRF has no Recenter, ToggleMap (dev map), snap turn, grab, holsters, body anchor, wrist display, comfort vignette or seated/standing mode.
- VRF has no weapon actor. Its firing is inside `AVRFPawn` and copies the rifle by reading `GetDefault<AVRTRifle>()` (fire interval, full-auto, projectile class, damage), so retuning the rifle retunes VRF. Unlimited ammo (step 12 does not apply to VRF).
- Damage lives in the projectile (`ApplyPointDamage` with the player controller as instigator), so enemies take damage the same way from both pawns.
- Logging: `LogVRTFlat` (possession, free camera on/off, zoom at Verbose, aim at VeryVerbose throttled). VRF shots log in `LogVRTWeapon`.

#### 11.5.1 Shared player base
- New `AVRTPlayerPawnBase` (`Player/VRTPlayerPawnBase.h/.cpp`): capsule, movement, health, walk/run speeds, shared death handling; virtual `PlayHapticPulseBothHands()`, `NotifyTeleported()`, `GetPawnViewLocation()`.
- `AVRTPawn` derives from it; the moved components keep their names. VR behaviour must not change.
- `VRTTriggerBase`, `VRTKey`, `VRTLevelExit` and `VRTGameMode::LoadPendingLevel` use `AVRTPlayerPawnBase*` instead of `AVRTPawn*`.
- `Weapons/VRTShot.h/.cpp`: `VRTShot::Fire(...)` spawns a projectile with owner, instigator and damage. `AVRTWeaponBase::Fire()` uses it. Public getters on `AVRTWeaponBase`: `GetFireInterval()`, `IsAutomatic()`, `GetProjectileClass()`, `GetProjectileDamage()`.
- **Test (headset):** VR plays exactly as before: keys, exit, pistol and gun, `VRT.Player.Damage`, death → restart.

#### 11.5.2 VRF pawn, controller, game mode: movement and camera
- `AVRFPawn`: body cylinder plus a "nose" mesh showing the facing, `USpringArmComponent` with absolute rotation (`CameraPitch` −90 by default, editable; fixed `CameraYaw`; no collision test) and `UCameraComponent`.
- WASD moves relative to the camera yaw (W = up on screen) with `UFloatingPawnMovement`, walk 150 / run 350 cm/s (Shift held). Walls block the capsule. No gravity.
- Scroll wheel zoom: arm length × 1.15 per notch, eased, clamped `MinArm` 400 to `MaxArm` 12000 cm (whole 32×32 maze visible).
- `GetPawnViewLocation()` = floor + `EyeHeight` (150 cm, same as `SeatedEyeHeight`).
- `AVRFPlayerController`: cursor visible, game-and-UI input mode (no mouse capture), `ShowFlag.Fog 0` at start.
- `AVRFGameMode : AVRTGameMode`: `DefaultPawnClass = AVRFPawn`, `PlayerControllerClass = AVRFPlayerController`, `HUDClass = AVRFHUD`.
- `VRF_*` mappings added to `Config/DefaultInput.ini` (see Input).
- **Test (monitor):** set the override on the test map and on the maze map; walk the maze from above; walls block; keys and exit work; next level moves the pawn to the new spawn; zoom in and out over a large maze.

#### 11.5.3 Aim and fire
- Mouse cursor → `DeprojectMousePositionToWorld` → intersection with the horizontal plane at muzzle height (`MuzzleHeight` 120 cm). The pawn turns to face that point; `VRT.Debug.Aim 1` draws the aim line.
- LMB held → full-auto through a timer and cooldown, fired through `VRTShot::Fire` with the rifle's interval, projectile class and damage.
- **Test (monitor):** `VRT.Dummy.Spawn`, shoot it: 10 damage per hit at 0.1 s, the same as the VR gun; shots stop at walls.

#### 11.5.4 Free camera and HUD
- `F` (`VRF_FreeCamera`) detaches the camera: it stays where it is while **WASD keeps moving the pawn**; only middle-mouse drag (`VRF_PanDrag` + `VRF_MouseX/Y`) pans the camera; the wheel still zooms. `F` again re-attaches it to the pawn. Aim and fire work in both modes.
- `AVRFHUD::DrawHUD` (no UMG): HP, keys X/4, exit status, level, seed, zoom, `FREE CAM` indicator; red screen flash on damage.
- **Test (monitor):** F detaches, middle-drag pans while WASD still walks the pawn, F returns; HUD values match the game state; `VRT.Player.Damage` flashes red; death restarts the level.

### Step 12 — Ammo and reload
- Pistol and gun magazines, reserve ammo.
- Reload gesture options:
  - simple: button press;
  - VR-style: bring the weapon to the holster, or tilt it down.
  - Decide later.
- Ammo pickups (trigger volumes, same base as keys). Ammo count on the wrist display and/or on the weapon.

### Step 13 — Enemy AI iterations (V0 → V1 → V2)
**Design source: `Docs/enemies.md`** (behaviour, parameters, decisions per enemy type). This step implements that file; it doesn't redesign it. Values marked [proposed] there are starting values for tuning.

Prerequisites: 10.5 (runtime NavMesh) and Step 11 (health and damage: melee hits on the player, enemy health).

Rules for the whole step:
- **Pure C++.** State machine in `AVRTEnemyController`. No Behavior Trees, no Blueprints. The V0/V1/V2 types are thin C++ subclasses that only set defaults, so the director can spawn them by class.
- **No animation.** Solid static meshes (engine primitives). All feedback in code: colour/emissive per state via a dynamic material instance, scale pulse, lean, flash before an attack, scale-out on death.
- **Walls block sight** both ways: "sees the player" = range + FOV + line trace, never distance alone.
- Logging in `LogVRTAI`: every state change as `Old -> New (reason)`, every perception event (seen, lost, heard shot, woke, alerted, was alerted).
- Debug cvar `VRT.Debug.AI 1`: sight cone and trace line (green clear / red blocked), last known location, wake spheres, alert radius, and the current state as world-space text above the enemy (visible in the HMD and in VRF).
- **Finding the player:** enemies get the player as `AVRTPlayerPawnBase` (`GetPlayerPawn(0)`) and trace sight to `GetPawnViewLocation()` (HMD in VR, floor + 150 cm in VRF), never to a camera. Never cast to `AVRTPawn` in AI code.
- Each sub-step is tested **first in VRF** (step 11.5: top-down view, debug drawing, on-screen text), with hand-placed enemies in `L_Test`, then in the maze; **then confirmed in the headset**.

#### 13.1 Enemy base
- `AVRTEnemy` (`APawn`): capsule root, `UStaticMeshComponent`, `UFloatingPawnMovement`, `UVRTHealthComponent`. Movement through `AAIController::MoveToLocation` / `MoveToActor` on the NavMesh.
- `AVRTEnemyController` (`AAIController`): `EVRTEnemyState` and a switch-based state machine. Timers instead of Tick where possible.
- Feature flags on the base class (`bCanHear`, `bCanAlert`). `AVRTEnemyV0`, `AVRTEnemyV1` and `AVRTEnemyV2` set them, plus their tunables.
- Collision: new object channel **Enemy** and profile `VRTEnemy` (blocks WorldStatic, Pawn and Projectile). Projectiles damage enemies.
- `VRT.Build.cs`: `AIModule`, `NavigationSystem`, `GameplayTasks`.
- **Test:** a placed enemy stands still, flashes on each hit, and scales out and disappears after 3 pistol hits.

#### 13.2 V0 senses and wandering
- Sight: own C++ check every 0.2 s (staggered between enemies). Range 15 m, FOV 120°, line trace from the enemy's eye to the player HMD, blocked by WorldStatic. 0.5 s grace before "lost".
- Wander: random reachable NavMesh point 8–12 m away, then a 1–3 s pause.
- **Test:** the enemy wanders. Stepping into its view logs "seen" and turns the debug line green. A wall between you and it blocks detection.

#### 13.3 V0 full state machine
- Wander → Chase (3.0 m/s) → Attack (melee only, 1.5 m, 10 damage every 1 s) → GoToLastKnown → Wait (4 s) → Wander. Seeing the player from any state → Chase.
- Gives up only at the last known location when the player is still not visible. No hearing, no alerting.
- Colour per state (e.g. grey Wander, red Chase, orange Wait). Flash before each hit.
- **Test:** get chased and hit, run around a corner, then watch V0 go to the spot where it lost you, wait there and resume wandering.

#### 13.4 Spawn modes: roaming and sleeping (V0, V1, V2)
- `EVRTSpawnMode { Roaming, Sleeping }`. Roaming starts in Wander.
- Gunshot events: every shot reports to a world subsystem from `VRTShot::Fire` (so VR weapons and the VRF pawn are both heard) (`UVRTNoiseSubsystem::ReportGunshot(LocationWorld)`, with a delegate). Sleeping enemies (and V1 in 13.5) listen to it. No `UAIPerceptionComponent`.
- Sleeping: no sight. Approach sphere 3 m → wake → Chase. Gunshot sphere 15 m → wake → go to the shot location. Spheres ignore walls. Wake-up delay 0.5–1 s with a visible cue. Never sleeps again; after waking it behaves as roaming.
- Sleeping visuals: dim colour, slow "breathing" scale pulse.
- **Test:** sneak past a sleeping V0 at more than 3 m without waking it. Walk inside 3 m and it wakes and chases. Shoot within 15 m (even behind a wall) and it wakes and goes to the shot spot.

#### 13.5 V1 — Listener
- V0 plus awake hearing of **gunshots only**, through walls, using the same gunshot events → goes to the shot location. Sight wins over hearing.
- Hearing range and redirect-on-new-shot: from `Docs/enemies.md` (still open there; decide before this sub-step).
- **Test:** shoot behind a wall out of its sight; V1 comes to the shot spot, waits, wanders.

#### 13.6 V2 — Social
- V0 plus alerting: on sighting the player, alerts **every enemy type** within a straight-line radius (walls ignored), passing the player's position. Alerted enemies go there (GoToLastKnown → Wait → Wander). Alerts don't chain and have no cooldown. Alerts also wake sleeping enemies in the radius.
- Visual/audio "shout" cue when it alerts.
- **Test:** let a V2 see you with a V0 and a sleeping V1 nearby but out of sight; both come to your position.

#### 13.7 AI performance check
- 20+ mixed enemies in a large maze: `stat unit`, `stat game`, AI tick cost. Sight checks staggered; enemies far from the player have tick and timers paused (prepares for the director's pool in Step 14).

### Step 14 — AI Director and spawning
- `AVRTDirector` (actor or `UWorldSubsystem`) reads the maze flow-distance field and the player's progress.
- Spawn rules:
  - out of the player's sight;
  - beyond a minimum distance;
  - in chunks ahead of or behind the player along the path.
- Uses the enemy pool.
- Chooses the enemy type (V0 / V1 / V2, more later) and spawn mode (roaming or sleeping) per spawn. Sleeping enemies suit dead ends, rooms and key cells (the player decides whether to risk sneaking past).
- Intensity model (L4D style): build-up → peak → relax, tracked from damage taken, kills and time. Spawns pause in the relax phase.
- Waves get triggered by events: picking up a key, getting near the exit with ≥ 2 keys.
- Per-level budget grows with level index and maze size.

### Step 15 — Combat polish
- Hit reactions and death without animation or ragdoll: flash, knock-back, dissolve or scale-out (enemies are static meshes).
- Muzzle flash, impact VFX (Niagara), sounds, enemy audio cues (important in a maze with 2 m walls).
- Balance: fire rates, damage, enemy health, enemy counts.

### Step 16 — Quest standalone
- Android packaging, Vulkan/Multi-View, fixed foveation, LODs, draw-call and enemy-count budget, profiling with RenderDoc / OVR Metrics.

---

## 6. Decisions (confirmed)
- Fire = **right index trigger** (the hand holding the weapon).
- Two-handed gun: releasing the **main (right) hand** → gun disappears and respawns over the right shoulder (left hand is freed too).
- Two-handed gun: releasing only the **left hand** → gun stays in the right hand, aim falls back to one-handed (planned default, not yet confirmed by playtest).
- Switching weapons: **release first**, then draw the other weapon.
- Keys 3 and 4: **no reward for now** (optional collectibles).
- Labyrinth: **no ceiling, open sky**, so beacons are visible from anywhere.
- Key status: **wrist display** on the left wrist.
- Development and testing are done **seated**: `AVRTPawn::PlayMode = Seated` (eye-level tracking, recentered, eyes at `SeatedEyeHeight` = 150 cm). Standing mode stays in the code; a runtime stand/seat switch comes later.
- Holsters follow the head yaw slowly (dead zone `BodyYawDeadZone` 35°, then `BodyYawFollowSpeed` 90°/s); snap turns rotate the body instantly.
- Enemy AI: **pure C++** (state machine, no Behavior Trees, no Blueprints where C++ can do it). **No animation**: enemies are solid static meshes. Enemy designs live in `Docs/enemies.md`.
- Enemy types: **V0** basic (sight, melee), **V1** + hearing gunshots, **V2** + alerting others. Each can spawn **roaming** or **sleeping**.
- **VRF flat playtest mode** (step 11.5) for AI and level work:
  - both player pawns derive from `AVRTPlayerPawnBase` (not an interface);
  - VRF firing lives in `AVRFPawn` (no weapon actor) and reads the rifle's defaults;
  - damage lives in the projectile (`ApplyPointDamage`);
  - VRF is chosen per map in **World Settings → GameMode Override** (no automatic HMD detection, no `?game=` alias);
  - camera pitch is editable, default −90° (pure top-down);
  - extras: **free camera only** (WASD keeps moving the pawn; middle-mouse drag pans the camera). No AI-ignore, god mode, teleport, time-scale keys.

## 7. Open questions
- None at the moment.

## 8. Progress
- [x] Step 1 — Cleanup and foundation
- [x] Step 2 — Test level
- [x] Step 3 — Hands: grab detection
- [x] Step 4 — Holsters
- [x] Step 5 — Weapon base and pistol
- [x] Step 6 — Two-handed gun
- [x] Step 7 — Keys and beacons
- [x] Step 8 — Level exit and game flow
- [ ] Step 9 — Integration, debug and performance pass
- [ ] Step 10 — Procedural labyrinth
  - [x] 10.1 Maze data, Growing Tree generator, braiding, rooms, spawn hub, seeded, debug draw (`VRT.Maze.*`)
  - [x] 10.2 `AVRTMazeBuilder`: chunked HISM walls, floor, PlayerStart
  - [x] 10.3 Key and beacon placement via BFS, exit at spawn
  - [x] 10.4 GameMode regenerates on next level (seed+1, bigger)
  - [x] 10.5 Runtime NavMesh rebuild
  - [ ] 10.6 Performance check on the max size
- [x] Step 11 — Health and damage
- [ ] Step 11.5 — VRF flat playtest mode
  - [x] 11.5.1 Shared player base (`AVRTPlayerPawnBase`, `VRTShot`)
  - [ ] 11.5.2 VRF pawn, controller, game mode: movement and camera
  - [ ] 11.5.3 Aim and fire
  - [ ] 11.5.4 Free camera and HUD
- [ ] Step 12 — Ammo and reload
- [ ] Step 13 — Enemy AI iterations (V0 → V1 → V2)
  - [ ] 13.1 Enemy base (pawn, controller, health, collision)
  - [ ] 13.2 V0 senses and wandering
  - [ ] 13.3 V0 full state machine
  - [ ] 13.4 Spawn modes: roaming and sleeping
  - [ ] 13.5 V1 — Listener
  - [ ] 13.6 V2 — Social
  - [ ] 13.7 AI performance check
- [ ] Step 14 — AI Director and spawning
- [ ] Step 15 — Combat polish
- [ ] Step 16 — Quest standalone
