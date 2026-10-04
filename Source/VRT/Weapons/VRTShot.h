#pragma once

#include "CoreMinimal.h"

class AVRTProjectile;

/**
 * The one place that turns a shot into a projectile. Used by the VR weapons and by the flat pawn (VRF), so damage,
 * ownership and logging are identical in both modes (and later gunshot noise for the AI is reported from here).
 */
namespace VRTShot
{
	/**
	 * Spawns a projectile at the muzzle, flying along the muzzle's forward (X) axis.
	 * Owner: the weapon (or the pawn); Instigator: the pawn that fires, so the damage goes to the right controller.
	 * Returns the projectile, or null if there is no class or the spawn failed.
	 */
	AVRTProjectile* Fire(UWorld* World, TSubclassOf<AVRTProjectile> ProjectileClass, const FTransform& MuzzleWorld, AActor* Owner, APawn* Instigator, float Damage);
}
