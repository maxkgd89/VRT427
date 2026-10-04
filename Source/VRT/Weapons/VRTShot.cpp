#include "Weapons/VRTShot.h"
#include "Engine/World.h"
#include "Weapons/VRTProjectile.h"
#include "VRTLog.h"

AVRTProjectile* VRTShot::Fire(UWorld* World, TSubclassOf<AVRTProjectile> ProjectileClass, const FTransform& MuzzleWorld, AActor* Owner, APawn* Instigator, float Damage)
{
	if (!World || !ProjectileClass)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Instigator;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AVRTProjectile* Projectile = World->SpawnActor<AVRTProjectile>(ProjectileClass, MuzzleWorld.GetLocation(), MuzzleWorld.GetRotation().Rotator(), Params);
	if (Projectile)
	{
		Projectile->SetDamage(Damage);
		VRT_LOG(LogVRTWeapon, Verbose, "Shot by %s (instigator %s): damage %.0f, muzzle(World)=%s dir(World)=%s", *GetNameSafe(Owner), *GetNameSafe(Instigator), Damage,
			*MuzzleWorld.GetLocation().ToCompactString(), *MuzzleWorld.GetRotation().GetForwardVector().ToCompactString());
	}
	return Projectile;
}
