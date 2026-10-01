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
**Keep it simple:** sight only, melee only, acts alone. Hearing → V1, alerting others → V2.

### Behaviour [decided]
1. **Wander** — moves around randomly while it hasn't seen the player.
2. **Sees player** → **Chase** — runs towards the player.
3. **In range** → **Attack**.
4. **Loses sight** (player ran away, walls broke line of sight) → runs to the player's **last known location**.
5. **At last known location** → **Wait** there for a while.
6. **Wait over** (player not seen again) → back to **Wander**.
- From any of Wander / GoToLastKnown / Wait: seeing the player again → Chase.
- **Giving up:** V0 gives up only after it reaches the last known location and still can't see the player. No max chase time.
- **Attack:** melee only.
- **Senses:** sight only. No hearing.
- **Social:** none. Does not alert or react to other enemies.

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
| Attack type | melee only | [decided] |
| Attack range | 1.5 m | [proposed] |
| Attack damage / rate | 10 dmg every 1.0 s | [proposed] |
| Health | 3 pistol hits | [proposed] |
| Hearing | none — sight only | [decided] |
| Alerting others | none | [decided] |
| Max chase time | none — gives up only at last known location | [decided] |

### Implementation notes (for the later plan)
- Pawn: `APawn` + capsule root + `UStaticMeshComponent` + `UFloatingPawnMovement`. Movement via `AAIController::MoveToLocation/MoveToActor` on NavMesh.
- Sight: own C++ check (trace) or `UAIPerceptionComponent` with sight config — decide in implementation plan. [open]
- Last known location = the player's position at the moment sight was lost (optionally plus a short extrapolation along their velocity). [proposed]
- Visual states (no animation): colour/emissive per state (e.g. grey Wander, red Chase, orange Wait), forward lean while moving, flash before attack. [proposed]
- Logging: one log line per state change in the enemy log category.

### Open questions
- None on behaviour. Proposed numbers to be tuned in playtests.

---

## V1 — Listener (draft)

**Role:** V0 with **very good hearing**. Walls block sight but not sound, so hiding behind a wall no longer makes you safe if you shoot.

### Behaviour [decided]
- Everything from V0, plus hearing.

### Ideas carried over from V0 discussion [proposed]
- Hears gunshots through walls (`ReportNoiseEvent` / own noise events from weapons).
- On hearing a noise → goes to the **noise location** (same as GoToLastKnown → Wait → Wander).
- Hearing range larger than sight range (e.g. 25–30 m); maybe also footsteps when the player runs.
- Sight still wins over hearing: if it sees the player → Chase.

### Open questions
1. What can it hear: gunshots only, or also footsteps / running?
2. Hearing range, and does it go through any number of walls?
3. Does hearing a new noise during Wait/GoToLastKnown redirect it?

---

## V2 — Social (draft)

**Role:** enemy that **alerts others**. Turns one sighting into a group threat.

### Behaviour [decided]
- Everything from V0, plus alerting other enemies.

### Ideas carried over from V0 discussion [proposed]
- On spotting the player → alerts enemies within a radius (e.g. 10–15 m, or same maze chunk), sending them the player's position.
- Alerted enemies go to that position (GoToLastKnown) even if they never saw the player.
- Possible audible/visual "shout" cue so the player knows they were reported.
- Optional: max chase time / give-up logic tuned for groups.

### Open questions
1. Who gets alerted: only V2s, or all enemy types?
2. Alert radius: distance, path distance through the maze, or line of sight between enemies?
3. Does the alert chain (alerted enemy alerts further)?
4. Is there a cooldown between alerts?

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
