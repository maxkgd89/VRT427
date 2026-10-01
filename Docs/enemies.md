# VRT — Enemy Database

Design notes for enemy types. **Not implemented yet.** This file is the source for a later enemy implementation plan.

Global decisions (apply to every enemy):
- Logic in C++ only (pure C++ state machine in the AI controller). No Behavior Trees, no Blueprints where C++ can do it.
- No animation. Enemies are solid static meshes; feedback is done in code (material flash, scale, bob, lean).
- The labyrinth has 2 m walls: walls block sight in both directions. Every "sees the player" check is a line-of-sight test, not just distance.

Status legend: **[decided]** = agreed · **[proposed]** = suggestion, not confirmed · **[open]** = needs a decision

---

## V0 — Basic chaser

**Role:** the first, simplest enemy. Teaches the player that enemies lose track of you behind walls.

### Behaviour [decided]
1. **Wander** — moves around randomly while it hasn't seen the player.
2. **Sees player** → **Chase** — runs towards the player.
3. **In range** → **Attack**.
4. **Loses sight** (player ran away, walls broke line of sight) → runs to the player's **last known location**.
5. **At last known location** → **Wait** there for a while.
6. **Wait over** (player not seen again) → back to **Wander**.
- From any of Wander / Search / Wait: seeing the player again → Chase.

### State machine
```
            sees player
 Wander ───────────────────► Chase ──in range──► Attack
   ▲                        ▲  │                   │
   │                        │  │lost sight         │out of range → Chase
   │          sees player   │  ▼                   │lost sight → GoToLastKnown
   │        ┌───────────────┴─ GoToLastKnown ◄─────┘
   │        │                    │ arrived
   │        │ sees player        ▼
   └──timer─┴──────────────── Wait
```

### Parameters
| Parameter | Value | Status |
|---|---|---|
| Sight range | 15 m (≈ 4 maze cells) | [proposed] |
| Sight cone (FOV) | 120° | [proposed] |
| Sight check | line trace from enemy "eye" to player HMD position, blocked by WorldStatic (walls) | [proposed] |
| Sight check rate | every 0.2 s (not every tick) | [proposed] |
| Lose-sight grace time | 0.5 s before switching to GoToLastKnown (avoids flicker at corners) | [proposed] |
| Wander speed | 1.0 m/s | [proposed] |
| Chase speed | 3.0 m/s (slower than player run 3.5 m/s, so the player can escape) | [proposed] |
| Wander target | random reachable NavMesh point within 8–12 m | [proposed] |
| Wander pause between points | 1–3 s | [proposed] |
| Wait at last known location | 4 s | [proposed] |
| Attack type | melee | [open] |
| Attack range | 1.5 m | [proposed] |
| Attack damage / rate | 10 dmg every 1.0 s | [proposed] |
| Health | 3 pistol hits | [proposed] |
| Hearing (gunshots) | none for V0 — sight only | [proposed] |

### Implementation notes (for the later plan)
- Pawn: `APawn` + capsule root + `UStaticMeshComponent` + `UFloatingPawnMovement`. Movement via `AAIController::MoveToLocation/MoveToActor` on NavMesh.
- Sight: own C++ check (trace) or `UAIPerceptionComponent` with sight config — decide in implementation plan. [open]
- Last known location = the player's position at the moment sight was lost (optionally plus a short extrapolation along their velocity). [proposed]
- Visual states (no animation): colour/emissive per state (e.g. grey Wander, red Chase, orange Wait), forward lean while moving, flash before attack. [proposed]
- Logging: one log line per state change in the enemy log category.

### Open questions
1. Melee only, or does V0 also have a ranged attack?
2. Does V0 react to gunshots it hears behind walls, or is that for a later version?
3. Several V0s: does one that spots the player alert others nearby?
4. Does V0 ever stop chasing on its own (max chase time), or only when it loses sight?

---

## Template for new enemies
```
## Vn — Name
Role:
Behaviour [decided]:
State machine:
Parameters (table with status):
Differences from previous version:
Implementation notes:
Open questions:
```
