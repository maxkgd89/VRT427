#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

// Custom channels, defined in Config/DefaultEngine.ini ([/Script/Engine.CollisionProfile]).
// GameTraceChannel1 is taken by the engine's "3DWidget" channel.
#define VRT_CHANNEL_PROJECTILE ECC_GameTraceChannel2 // object channel
#define VRT_CHANNEL_GRAB ECC_GameTraceChannel3 // trace channel (optional; overlaps are usually enough)

namespace VRTCollision
{
	/** Player capsule: blocks WorldStatic/WorldDynamic, ignores projectiles. */
	static const FName PlayerProfile(TEXT("VRTPlayer"));
	/** Pickup/exit triggers: overlap Pawn only. */
	static const FName TriggerProfile(TEXT("VRTTrigger"));
	/** Projectiles: block WorldStatic, WorldDynamic and Pawn; ignore the player (via VRTPlayer). */
	static const FName ProjectileProfile(TEXT("VRTProjectile"));
	/** Held weapons: no collision. */
	static const FName WeaponProfile(TEXT("VRTWeapon"));
	/** Hand grab sphere: overlaps grab points (WorldDynamic) only. */
	static const FName HandProfile(TEXT("VRTHand"));
	/** Grab points (holster zones, barrel grip, debug targets): overlap hand spheres only. */
	static const FName GrabPointProfile(TEXT("VRTGrabPoint"));
}
