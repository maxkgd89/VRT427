# AGENTS.md — VRT (UE 4.27 VR labyrinth shooter)

Instructions for any coding agent working in this repository (Claude Code reads it via `CLAUDE.md`).

## Project at a glance
- VR shooter in a labyrinth: find keys guided by blue beacons, return to spawn with ≥ 2 of 4 keys, go to the next level. A pistol (one hand) and a gun (two hands) are drawn from body holsters.
- **The full plan and the current progress live in `Docs/plan.md`. Read it before starting any task**, and work on one step at a time.
- Unreal Engine **4.27**, **C++ first**, OpenXR. Target: PC VR (Rift S, Quest 3 via Link) first, Quest standalone later.
- Paths:
  - project `E:\UE_projects\VRT`
  - engine `E:\UE_4.27` (use it to look up engine headers and source, e.g. `Engine\Source\Runtime\...`)

## Repository layout
```
VRT.uproject
AGENTS.md, CLAUDE.md        agent instructions
Docs/plan.md                development plan + progress checklist
Config/                     Default*.ini (input, collision, rendering, maps)
Source/VRT/
  VRTLog.h/.cpp             log categories (see Logging)
  Player/                   VRTPawn, hands, holsters
  Weapons/                  weapon base, pistol, rifle, projectile
  Gameplay/                 game mode/state, triggers, keys, beacons, exit
  Level/                    maze data, generator, builder (step 10)
  AI/  Combat/  UI/  Debug/ later steps
Content/VRT/                our assets (maps, materials, meshes)
Content/VRTemplate/         UE VR Template — reference only
```

## Hard rules
- **Do not use the VR Template's gameplay**: `VRPawn`, `GrabComponent`, `VRInteractionBPI`, `Pistol`, `Projectile`, `Menu`, `VRGameMode` BPs. Read them for ideas only; all our gameplay is C++.
- **Never edit binary assets** (`.uasset`, `.umap`) as files. When something must be done in the editor, give the user short, numbered editor steps.
- **Test level naming:** `Tools/Editor/create_test_level.py` generates the test map. Each time the script changes, bump `MAP_NAME` to the next number (`L_Test2`, `L_Test3`, ...) and keep older maps; tell the user to point the default maps (`Config/DefaultEngine.ini`) at the new one after it exists.
- Never touch `Binaries/`, `Intermediate/`, `DerivedDataCache/`. Read `Saved/Logs/` freely; don't edit `Saved/`.
- Weapons never simulate physics. Released weapons hide and respawn in their holster; they are never destroyed.
- Movement is horizontal only (no gravity or jumping); walls block the pawn's capsule.
- Do not commit or push unless the user asks. Propose a commit message at the end of each step.

## Building
- Close the editor or turn off Live Coding before a command-line build. Otherwise UBT refuses ("Live Coding is active").
- Editor build:
  ```
  "E:\UE_4.27\Engine\Build\BatchFiles\Build.bat" VRTEditor Win64 Development -Project="E:\UE_projects\VRT\VRT.uproject" -WaitMutex -FromMsBuild
  ```
- After adding or removing source files, regenerate project files (right-click `VRT.uproject` → *Generate Visual Studio project files*, or `UnrealBuildTool -projectfiles`).
- Fix all warnings in our code; don't leave new warnings behind.

## C++ conventions
- Epic coding standard. Class prefixes `A/U/F/E/I` + `VRT`, e.g. `AVRTPistol`, `UVRTHandComponent`, `EVRTGripState`.
- Headers: forward-declare, include what you use, `#pragma once`, `GENERATED_BODY()`.
- Every `UObject*` member is a `UPROPERTY()`. Tunables are `EditAnywhere` with categories `VRT|Locomotion`, `VRT|Weapon`, `VRT|TwoHand`, etc.
- Units: centimetres, degrees, seconds. Write units in property comments.
- Name the space of every vector in code and logs: **World**, **PawnLocal** (relative to the VROrigin) or **Local** (component).
- Prefer events and delegates over Tick. When Tick is needed, keep it cheap (Quest budget).
- Debug console variables live under `VRT.Debug.*` (e.g. `VRT.Debug.Triggers`, `VRT.Debug.Holsters`, `VRT.Debug.Grab`, `VRT.Debug.TwoHand`). Debug drawing only happens when its cvar is on.
- Input uses **named mappings** from `Config/DefaultInput.ini`: `GrabLeft`, `GrabRight`, `FireRight` (right index trigger), `MoveX`, `MoveY`, `Turn`, `ToggleRun` (left X button). Never bind `EKeys::OculusTouch_*` directly in new code.

## Logging

Each system and plan step has its **own log category**, so a problem can be isolated by filtering one category in `Saved/Logs/VRT.log`.

### Categories (declared in `Source/VRT/VRTLog.h`)
| Category | Plan step | What it logs |
|---|---|---|
| `LogVRT` | all | Module startup, general messages that fit nowhere else |
| `LogVRTInput` | 1 | Binding setup, action presses/releases (Verbose), raw axes (VeryVerbose) |
| `LogVRTPawn` | 1–2 | Locomotion, run toggle, snap turn, capsule-follows-HMD corrections, tracking origin |
| `LogVRTHand` | 3 | Grab candidates in range, grip press/release, chosen target, held item changes |
| `LogVRTHolster` | 4 | Body-anchor update, zone enter/exit, draw from holster, respawn into holster |
| `LogVRTWeapon` | 5–6 | Attach/detach, fire, fire-rate cooldown, muzzle transform, return-to-holster |
| `LogVRTTwoHand` | 6 | Two-hand grip state machine and aim rotation math (see below) |
| `LogVRTProjectile` | 5 | Spawn (pos/dir/speed), hit (actor, component, location), lifetime expiry |
| `LogVRTGameFlow` | 7–8 | Key pickup, key count, beacon on/off, exit check, level state transitions, fades |
| `LogVRTMaze` | 10 | Seed, grid size, algorithm params, braiding/rooms stats, key cells, build time, instance counts |
| `LogVRTNav` | 10–13 | NavMesh rebuild start/end/time, path failures |
| `LogVRTAI` | 13 | Enemy state transitions, perception events, move requests and results |
| `LogVRTDirector` | 14 | Intensity, spawn decisions (cell, reason, rejected cells), budget |
| `LogVRTCombat` | 11–12 | Damage (instigator, amount, remaining HP), death, ammo, reload |

A new system gets a new category. Add it to this table and to `VRTLog.h/.cpp` in the same change.

### Code
`Source/VRT/VRTLog.h`
```cpp
#pragma once
#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVRT, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTInput, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTPawn, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTHand, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTHolster, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTWeapon, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTTwoHand, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTProjectile, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTGameFlow, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTMaze, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTNav, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTDirector, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTCombat, Log, All);

// Prefixes every line with function and line. Pass a plain string literal (no TEXT()).
#define VRT_LOG(Category, Verbosity, Format, ...) \
	UE_LOG(Category, Verbosity, TEXT("[%s:%d] ") TEXT(Format), ANSI_TO_TCHAR(__FUNCTION__), __LINE__, ##__VA_ARGS__)

// For per-frame data: logs at most once per IntervalSec per call site, and only if the category is enabled.
#define VRT_LOG_THROTTLED(Category, Verbosity, IntervalSec, Format, ...) \
	do { \
		if (UE_LOG_ACTIVE(Category, Verbosity)) \
		{ \
			static double VRT_LastLogTime = -1000.0; \
			const double VRT_Now = FPlatformTime::Seconds(); \
			if (VRT_Now - VRT_LastLogTime >= (IntervalSec)) \
			{ \
				VRT_LastLogTime = VRT_Now; \
				VRT_LOG(Category, Verbosity, Format, ##__VA_ARGS__); \
			} \
		} \
	} while (0)
```
`Source/VRT/VRTLog.cpp`: one `DEFINE_LOG_CATEGORY(LogVRTxxx);` per category.

Usage:
```cpp
VRT_LOG(LogVRTWeapon, Log, "%s attached to %s", *GetName(), *Hand->GetName());
VRT_LOG_THROTTLED(LogVRTTwoHand, VeryVerbose, 0.25, "Axis=%s Dist=%.1f", *Axis.ToCompactString(), Dist);
```

### Verbosity rules
- `Error`: broken invariant (null where impossible, asset missing). Should never happen.
- `Warning`: recoverable but suspicious (degenerate math, fallback used, grab refused unexpectedly).
- `Log`: **state changes and one-off events** (grabbed, released, fired, key picked, level complete). Default level; keep it readable.
- `Verbose`: extra detail on events (candidate lists, transforms at the moment of an event).
- `VeryVerbose`: **per-frame data**. Always throttled with `VRT_LOG_THROTTLED`.
- Format state changes as `Old -> New (reason)`. Always name the space of vectors (World / PawnLocal / Local).

### Controlling verbosity
- Defaults in `Config/DefaultEngine.ini`:
  ```ini
  [Core.Log]
  LogVRT=Log
  LogVRTTwoHand=Log
  ```
- At runtime (console `~` in the editor or the spectator window): `Log LogVRTTwoHand VeryVerbose`, then back with `Log LogVRTTwoHand Log`.
- From the command line: `-LogCmds="LogVRTTwoHand VeryVerbose, LogVRTHand Verbose"`.
- The log file is `Saved/Logs/VRT.log`. Earlier runs are kept as `VRT-backup-*.log`.
- On-screen debug messages are not visible in the HMD. Rely on the log file plus world debug drawing (`VRT.Debug.*`).

### Agent debugging workflow
1. The user describes the problem and plays a session with the relevant category at `Verbose` or `VeryVerbose`.
2. The agent reads `Saved/Logs/VRT.log`, filtered by that category (e.g. search for `LogVRTTwoHand:`), and finds the first Warning or the last state change before the bug.
3. If the logs don't explain it, the agent adds targeted `Verbose` logs, asks for one more run, and then removes or downgrades noisy logs once the bug is fixed.

### Two-handed gun (step 6): what `LogVRTTwoHand` must cover
Grip state machine `EVRTGripState { None, OneHand, TwoHand }`:
- `Log`, on every transition: `OneHand -> TwoHand (left grabbed barrel)`, `TwoHand -> OneHand (left released)`, `* -> None (right released → returning to shoulder)`. Include the hand-to-hand distance at the moment of transition.
- `Log`, on entering TwoHand: right grip location (PawnLocal), left hand location (PawnLocal), barrel grip point (Local to gun), initial distance.
- `VeryVerbose`, throttled about 0.25 s, each frame in TwoHand:
  - `R` and `L` hand locations (PawnLocal);
  - `Axis = (L - R).GetSafeNormal()`, hand distance;
  - `UpRef` = right controller up vector, `Dot(Axis, UpRef)`;
  - resulting gun rotation (Pitch / Yaw / Roll);
  - angular change since the last frame.
- `Warning` (log once per occurrence, not every frame):
  - hand distance < `MinTwoHandDistance` (about 10 cm) → fall back to one-hand aim;
  - `|Dot(Axis, UpRef)| > 0.95` → up vector nearly parallel to the aim axis, roll unstable, previous frame's up used;
  - rotation jumps more than 45° in one frame (flip or pop);
  - left hand drifts more than X cm from the barrel grip point (stretching).
- Debug draw with `VRT.Debug.TwoHand 1`: sphere at each grip point, line R→L, coordinate axes on the gun (X red forward, Y green, Z blue), and the right controller's up vector.
- Typical causes to check first:
  - mixing World and PawnLocal space;
  - the gun mesh's forward axis not being +X;
  - OpenXR grip pose vs aim pose (motion sources `Right` vs `RightAim`);
  - roll flips when UpRef is near the axis;
  - attachment rules changing scale;
  - snap turn while holding the gun two-handed.

## Testing
- Each plan step ends with a **headset test** listed in `Docs/plan.md`. After finishing a step, write short test instructions for the user (what to do, what should happen, which log category to watch).
- Use VR Preview in the editor. Remember that `bStartInVR=True`.
- After the user confirms a test passed, tick the step in `Docs/plan.md` → *Progress*.
