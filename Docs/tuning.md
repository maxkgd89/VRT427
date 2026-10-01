# VRT tuning reference

Generated from the `EditAnywhere` properties in `Source/VRT` (defaults as coded). Units: cm, degrees, seconds.
Change a value in the header (default for the class) or on the placed actor / Blueprint-less pawn in the editor details panel.


## `AVRTBeacon`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `Height` | `1500.f` | VRT > Beacon | Column height in cm (10 to 20 m). |
| `Radius` | `25.f` | VRT > Beacon | Column radius in cm (25 = 0.5 m wide). |
| `Color` | `FLinearColor(0.05f, 0.3f, 4.f, 1.f)` | VRT > Beacon | Emissive colour of the column (linear, values above 1 glow). |

## `AVRTGameMode`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `FadeInSeconds` | `1.f` | VRT > GameFlow | Seconds of the fade from black when a level starts. |
| `FadeOutSeconds` | `1.5f` | VRT > GameFlow | Seconds of the fade to black when a level is completed. |

## `AVRTKey`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `BeaconOffset` | `FVector2D(60.f, 0.f)` | VRT > Key | Where the beacon stands relative to the key, horizontal, cm (World axes). |
| `PickupSound` | `nullptr` | VRT > Key |  |
| `PickupSoundMaxSeconds` | `1.f` | VRT > Key | A pickup sound that loops or is longer than this is cut off after this many seconds. |
| `PickupHapticIntensity` | `0.7f` | VRT > Key | Haptic pulse on both hands at pickup (0-1). |
| `BeaconFadeSeconds` | `1.f` | VRT > Key | Seconds the beacon takes to fade out. |
| `SpinSpeed` | `90.f` | VRT > Key | Spin speed of the key, degrees per second. |

## `AVRTLevelExit`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `RequiredKeys` | `2` | VRT > Exit | Keys needed to leave. |
| `ClosedColor` | `FLinearColor(3.f, 0.1f, 0.05f, 1.f)` | VRT > Exit | Pad colour while the exit is closed / open (linear, values above 1 glow). |
| `OpenColor` | `FLinearColor(0.1f, 3.f, 0.3f, 1.f)` | VRT > Exit |  |
| `DeniedHapticIntensity` | `0.6f` | VRT > Exit | Buzz on both hands when the player enters without enough keys (0-1). |
| `FloorOffset` | `85.f` | VRT > Exit | Distance from the actor origin down to the floor, cm. The pad sits on the floor. |

## `AVRTTriggerBase`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `TriggerRadius` | `90.f` | VRT > Trigger | Trigger radius in cm. |
| `bShowDebugSphere` | `false` | VRT > Trigger | Draw the trigger sphere even when VRT.Debug.Triggers is off. |
| `DebugColor` | `FColor::Cyan` | VRT > Trigger | Colour of the debug sphere. |

## `UVRTGrabPointComponent`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `AllowedHand` | `EVRTHandFilter::Any` | VRT > Grab |  |
| `bGrabEnabled` | `true` | VRT > Grab |  |
| `GrabRadius` | `15.f` | VRT > Grab | Grab zone radius in cm. |

## `UVRTHandComponent`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `Hand` | `EControllerHand::Left` | VRT > Hand | Which hand this component represents. Set by the owning pawn. |
| `GrabRadius` | `8.f` | VRT > Hand | Overlap sphere radius in cm. |

## `UVRTHolsterComponent`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `HeightFraction` | `1.f` | VRT > Holster | Zone height = HeightFraction * head height + HeightOffset. |
| `HeightOffset` | `0.f` | VRT > Holster | Added to the zone height, cm. |
| `ForwardOffset` | `0.f` | VRT > Holster | Cm in front of (+) or behind (-) the body anchor. |
| `RightOffset` | `0.f` | VRT > Holster | Cm to the right of (+) or left of (-) the body anchor. |

## `AVRTPawn`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `SnapTurnAngle` | `30.f` | VRT > Locomotion |  |
| `SnapTurnActivationThreshold` | `0.6f` | VRT > Locomotion | Stick deflection needed to trigger a turn. |
| `SnapTurnResetThreshold` | `0.3f` | VRT > Locomotion | Stick must return below this before another turn can trigger. |
| `MoveDeadZone` | `0.2f` | VRT > Locomotion | Stick deflection below this is ignored (stick drift). Output is rescaled so it still ramps from 0 to 1. |
| `PlayMode` | `EVRTPlayMode::Seated` | VRT > Locomotion | Standing uses floor-level tracking. Seated uses eye-level tracking and lifts the origin to SeatedEyeHeight. |
| `SeatedEyeHeight` | `150.f` | VRT > Locomotion | Eye height above the floor in seated mode, cm. |
| `FireAction` | `FName("FireRight")` | VRT > Weapon | Input action that fires the weapon in the right hand. Remap here (e.g. to "FireLeft") without other changes. |
| `bComfortVignette` | `false` | VRT > Comfort | Darkens the screen edges while moving to reduce motion sickness. Off by default; costs a post-process pass. |
| `VignetteMaxIntensity` | `0.7f` | VRT > Comfort | Vignette strength while moving at full stick deflection (0-1). |
| `VignetteFadeSpeed` | `4.f` | VRT > Comfort | How quickly the vignette fades in and out (higher is faster). |
| `WalkSpeed` | `150.f` | VRT > Locomotion | Walk speed in cm/s. |
| `RunSpeed` | `350.f` | VRT > Locomotion | Run speed in cm/s, used while in run mode. |
| `CapsuleRadius` | `30.f` | VRT > Collision |  |
| `CapsuleHalfHeight` | `85.f` | VRT > Collision |  |
| `GravityZ` | `-980.f` | VRT > Collision | Downward acceleration in cm/s^2, so the capsule settles back to the floor after stepping onto something. |
| `BodyYawDeadZone` | `35.f` | VRT > Holster | The body (holsters) only starts turning once the head is turned further than this from it, degrees. |
| `BodyYawFollowSpeed` | `90.f` | VRT > Holster | How fast the body turns after the head has left the dead zone, degrees per second. Snap turns rotate it instantly. |

## `AVRTProjectile`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `Speed` | `4500.f` | VRT > Weapon | Speed in cm/s. |
| `Lifetime` | `3.f` | VRT > Weapon | Seconds before the projectile removes itself. |
| `HitMarkerRadius` | `6.f` | VRT > Weapon | Radius of the debug flash drawn at the impact point, cm. |

## `AVRTRifle`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `MinTwoHandDistance` | `10.f` | VRT > TwoHand | Below this hand-to-hand distance the aim line is unreliable and the gun follows the right controller, cm. |
| `StretchWarnDistance` | `20.f` | VRT > TwoHand | Warn if the left hand is further than this from the barrel grip point, cm. |
| `JumpWarnAngle` | `45.f` | VRT > TwoHand | Warn if the gun rotates more than this in one frame, degrees. |

## `AVRTWeaponBase`

| Property | Default | Category | Meaning |
|---|---|---|---|
| `FireSound` | `nullptr` | VRT > Weapon |  |
| `FireSoundMaxSeconds` | `0.25f` | VRT > Weapon | A fire sound that is looping or longer than this is cut off after this many seconds. |
| `bAutomatic` | `false` | VRT > Weapon | True: holding the trigger keeps firing every FireInterval. False: one shot per press. |
| `FireInterval` | `0.4f` | VRT > Weapon | Seconds between shots. |
| `GripLocation` | `FVector::ZeroVector` | VRT > Weapon | Weapon position relative to the hand sphere while held, cm. |
| `GripRotation` | `FRotator::ZeroRotator` | VRT > Weapon | Weapon rotation relative to the hand sphere while held. |
| `HolsterLocation` | `FVector::ZeroVector` | VRT > Weapon | Weapon position relative to the holster zone while holstered, cm. |
| `HolsterRotation` | `FRotator::ZeroRotator` | VRT > Weapon |  |
| `FireHapticIntensity` | `0.8f` | VRT > Weapon | Haptic pulse on the holding hand when a shot is fired (0-1). |

## Set in constructors (not as property defaults)

`AVRTPawn` constructor (`Source/VRT/Player/VRTPawn.cpp`):

| Holster | HeightFraction | HeightOffset | ForwardOffset | RightOffset | Weapon |
|---|---|---|---|---|---|
| `WaistHolster` | 0.55 | 8 | 5 | 20 | `AVRTPistol` |
| `ShoulderHolster` | 1.0 | -10 | -10 | 15 | `AVRTRifle` |

Pistol: grip offset `(8, 0, 0)`, 0.4 s fire interval, semi-auto. Gun: grip offset `(12, 0, 0)`, 0.1 s fire interval, full-auto, barrel grip point at `(12, 0, 0)` with a 10 cm radius.
Hand grab sphere: 8 cm. Holster zones: 18 cm. Projectile: 4500 cm/s, 3 s lifetime, 4 cm ball.
