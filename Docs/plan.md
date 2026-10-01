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

---

## 2. Target architecture

### Source layout
```
Source/VRT/
  Player/      VRTPawn, VRTHandComponent, VRTHolsterComponent
  Weapons/     VRTWeaponBase, VRTPistol, VRTRifle, VRTProjectile
  Gameplay/    VRTGameMode, VRTGameState, VRTKey, VRTBeacon, VRTLevelExit, VRTTriggerBase
  Level/       VRTMazeGenerator, VRTMazeData, VRTMazeBuilder        (step 10)
  AI/          VRTEnemy, VRTEnemyController, VRTDirector            (step 11+)
  Combat/      VRTHealthComponent, VRTDamageTypes                   (step 11+)
  UI/          VRTWristDisplay                                      (step 8–9)
  Debug/       VRTDebugSettings (console vars)
```

### Key classes
| Class | Base | Role |
|---|---|---|
| `AVRTPawn` | APawn | Exists already. HMD, controllers, capsule, smooth move, snap turn. Will gain hands and holsters. |
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
- **Wrist display** (left wrist `UWidgetComponent`): keys X/4, the exit requirement, level number.
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

### Step 12 — Ammo and reload
- Pistol and gun magazines, reserve ammo.
- Reload gesture options:
  - simple: button press;
  - VR-style: bring the weapon to the holster, or tilt it down.
  - Decide later.
- Ammo pickups (trigger volumes, same base as keys). Ammo count on the wrist display and/or on the weapon.

### Step 13 — Enemy base
- `AVRTEnemy` (ACharacter, CharacterMovement on NavMesh) plus `AVRTEnemyController` (AAIController).
- Logic: C++ finite state machine or a Behavior Tree with C++ tasks and services. **StateTree is not available in 4.27.** Recommendation: BT plus C++ tasks, or a pure C++ FSM for full control.
- `UAIPerceptionComponent`: sight (walls block it), hearing (gunshots via `ReportNoiseEvent`).
- States: Idle / Wander → Alert → Chase → Attack (melee or ranged) → Search → Return.
- First enemy types: a melee rusher (L4D "common") and a ranged shooter. Later, specials.

### Step 14 — AI Director and spawning
- `AVRTDirector` (actor or `UWorldSubsystem`) reads the maze flow-distance field and the player's progress.
- Spawn rules:
  - out of the player's sight;
  - beyond a minimum distance;
  - in chunks ahead of or behind the player along the path.
- Uses the enemy pool.
- Intensity model (L4D style): build-up → peak → relax, tracked from damage taken, kills and time. Spawns pause in the relax phase.
- Waves get triggered by events: picking up a key, getting near the exit with ≥ 2 keys.
- Per-level budget grows with level index and maze size.

### Step 15 — Combat polish
- Hit reactions, death, ragdoll or simple dissolve.
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
- Holsters follow the head yaw slowly (dead zone `BodyYawDeadZone` 35�, then `BodyYawFollowSpeed` 90�/s); snap turns rotate the body instantly.

## 7. Open questions
- None at the moment.

## 8. Progress
- [x] Step 1 — Cleanup and foundation
- [x] Step 2 — Test level
- [x] Step 3 — Hands: grab detection
- [x] Step 4 — Holsters
- [x] Step 5 — Weapon base and pistol
- [ ] Step 6 — Two-handed gun
- [ ] Step 7 — Keys and beacons
- [ ] Step 8 — Level exit and game flow
- [ ] Step 9 — Integration, debug and performance pass
- [ ] Step 10 — Procedural labyrinth
- [ ] Step 11+ — Combat and AI
