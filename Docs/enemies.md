# VRT — Enemy Database

Design notes for enemy types. **Not implemented yet.** This file is the source for a later enemy implementation plan.

Global decisions (apply to every enemy):
- Logic in C++ only (pure C++ state machine in the AI controller). No Behavior Trees, no Blueprints where C++ can do it.
- No animation. Enemies are solid static meshes; feedback is done in code (material flash, scale, bob, lean).
- The labyrinth has 2 m walls: walls block sight in both directions. Every "sees the player" check is a line-of-sight test, not just distance.

Status legend: **[decided]** = agreed · **[proposed]** = suggestion, not confirmed · **[open]** = needs a decision

---

## Spawn modes (shared by enemy types)

Applies to: **V0, V1, V2** [decided]

Every enemy can be spawned in one of two modes.

### 1) Roaming [decided]
- Spawns awake and starts in **Wander**. Uses its normal senses (sight, plus hearing if the type has it).

### 2) Sleeping [decided]
- Spawns asleep, standing still.
- **No sight** while asleep.
- Perception while asleep is **imitated with two spheres** around the enemy (simple distance check, walls ignored, no line-of-sight):
  - **Approach sphere (3 m)**: the player enters it → enemy wakes → **Chase**.
  - **Gunshot sphere (15 m)**: a gunshot inside it → enemy wakes → goes to the **shot location** (GoToLastKnown → Wait → Wander).
- Applies to V0 too: a sleeping V0 wakes on gunshots via the sphere, even though an awake V0 has no hearing.
- **Wake-up delay** with a visible "waking" cue, so the player gets a moment to react.
- A **V2 alert wakes** sleeping enemies inside the alert radius (they go to the alerted position).
- **Never sleeps again**: after waking, the enemy uses its normal roaming behaviour for the rest of its life.
- Design intent: the player can **choose to risk** sneaking past a sleeping enemy close to it, as long as they stay out of the approach sphere and don't shoot nearby.

### Sleeping parameters
| Parameter | Value | Status |
|---|---|---|
| Approach sphere radius | 3 m | [decided] |
| Gunshot sphere radius | 15 m | [decided] |
| Spheres blocked by walls | no — pure spheres | [decided] |
| Wake-up delay | 0.5–1 s | [decided] (exact value tuned in playtest) |
| Visual while asleep | dim colour / no emissive, slow "breathing" scale pulse | [proposed] |
| Visual while waking | brightening colour / scale pop | [proposed] |

### Open questions
- None.

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
- Hears **gunshots only** (no footsteps, no other sounds).

### Ideas [proposed]
- Gunshots heard through walls (weapons report a noise event on fire).
- On hearing a gunshot → goes to the **shot location** (same as GoToLastKnown → Wait → Wander).
- Hearing range larger than sight range (e.g. 25–30 m).
- Sight still wins over hearing: if it sees the player → Chase.

### Open questions
1. Hearing range, and is it a plain sphere through walls (like sleeping mode) or reduced by walls?
2. Does hearing a new gunshot during Wait/GoToLastKnown redirect it?

---

## V2 — Social (draft)

**Role:** enemy that **alerts others**. Turns one sighting into a group threat.

### Behaviour [decided]
- Everything from V0, plus alerting other enemies.
- Alerts **every enemy type**.
- Alert reach = **straight-line distance** (radius around the V2; not path distance, walls ignored).
- Alerts **do not chain**: an alerted enemy does not pass the alert on.
- **No cooldown** between alerts.
- Alerts also **wake sleeping enemies** in the radius.

### Ideas [proposed]
- Triggered when the V2 spots the player; sends the player's current position.
- Alerted enemies go to that position (GoToLastKnown → Wait → Wander) even if they never saw the player.
- Alert radius 10–15 m.
- Audible/visual "shout" cue so the player knows they were reported.
- Optional: max chase time / give-up logic tuned for groups.

### Open questions
1. With no cooldown: alert once per sighting, or repeatedly while it keeps seeing the player (e.g. every sight check updates the others)?
2. Alert radius value.

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
