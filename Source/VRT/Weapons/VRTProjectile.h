#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRTProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/** Small straight-flying bullet: no gravity, short lifetime, destroyed on the first hit. */
UCLASS()
class VRT_API AVRTProjectile : public AActor
{
	GENERATED_BODY()

public:
	AVRTProjectile();

public:
	/** Sets the damage dealt on a hit. The weapon calls this right after spawning the bullet. */
	void SetDamage(float InDamage) { Damage = InDamage; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	UProjectileMovementComponent* Movement;

	/** Speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon", meta = (ClampMin = "100.0"))
	float Speed = 4500.f;

	/** Damage dealt to whatever it hits (point damage through the UE damage flow). The weapon overrides it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon", meta = (ClampMin = "0.0"))
	float Damage = 20.f;

	/** Seconds before the projectile removes itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon", meta = (ClampMin = "0.1"))
	float Lifetime = 3.f;

	/** Radius of the debug flash drawn at the impact point, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	float HitMarkerRadius = 6.f;
};
