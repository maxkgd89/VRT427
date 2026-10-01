#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRTWeaponBase.generated.h"

class AVRTProjectile;
class UStaticMeshComponent;
class USoundBase;
class UVRTHandComponent;
class UVRTHolsterComponent;

/**
 * Base for the pistol and the gun. A weapon never simulates physics and is never destroyed: it lives in its
 * holster, is attached to the hand while drawn, and goes back to the holster when released.
 */
UCLASS(Abstract)
class VRT_API AVRTWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AVRTWeaponBase();

	/** Tells the weapon which holster it belongs to and parks it there. */
	void SetHolster(UVRTHolsterComponent* InHolster);

	/** Attaches to the hand at the grip offset. Called when the hand draws the weapon. */
	void AttachToHand(UVRTHandComponent* Hand);

	/** Detaches from the hand, hides, moves back into the holster and shows again. */
	void ReturnToHolster();

	/** Trigger input from the holding hand. */
	virtual void OnTriggerPressed();
	virtual void OnTriggerReleased();

	UVRTHandComponent* GetHoldingHand() const { return HoldingHand; }
	bool IsHeld() const { return HoldingHand != nullptr; }

protected:
	/** Fires if the cooldown allows it. Returns true if a shot was fired. */
	bool TryFire();

	/** Spawns the projectile at the muzzle. */
	virtual void Fire();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	UStaticMeshComponent* Mesh;

	/** Projectiles spawn here and fly along its forward (X) axis. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	USceneComponent* Muzzle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	TSubclassOf<AVRTProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	USoundBase* FireSound = nullptr;

	/** A fire sound that is looping or longer than this is cut off after this many seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon", meta = (ClampMin = "0.1"))
	float FireSoundMaxSeconds = 0.5f;

	/** Seconds between shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon", meta = (ClampMin = "0.01"))
	float FireInterval = 0.4f;

	/** Weapon position relative to the hand sphere while held, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	FVector GripLocation = FVector::ZeroVector;

	/** Weapon rotation relative to the hand sphere while held. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	FRotator GripRotation = FRotator::ZeroRotator;

	/** Weapon position relative to the holster zone while holstered, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	FVector HolsterLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	FRotator HolsterRotation = FRotator::ZeroRotator;

	/** Haptic pulse on the holding hand when a shot is fired (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FireHapticIntensity = 0.8f;

private:
	UPROPERTY(Transient)
	UVRTHandComponent* HoldingHand = nullptr;

	UPROPERTY(Transient)
	UVRTHolsterComponent* Holster = nullptr;

	/** World time of the last shot (seconds). */
	float LastFireTime = -1000.f;
};
