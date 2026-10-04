#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VRTHealthComponent.generated.h"

class AController;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FVRTDamagedSignature, float, Damage, float, NewHealth, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVRTDeathSignature, AActor*, DamageCauser);

/**
 * Health for the player and for enemies. Listens to the owner's OnTakeAnyDamage, so everything that goes through the
 * UE damage flow (UGameplayStatics::ApplyDamage / ApplyPointDamage, AActor::TakeDamage) hurts the owner.
 */
UCLASS(ClassGroup = "VRT", meta = (BlueprintSpawnableComponent))
class VRT_API UVRTHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVRTHealthComponent();

	/** Back to full health and alive again (new level, respawn). */
	void ResetHealth();

	/** Adds health, capped at MaxHealth. Ignored when dead. */
	void Heal(float Amount);

	bool IsAlive() const { return Health > 0.f; }
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }

	/** 0 (dead) to 1 (full). */
	float GetHealthFraction() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

	/** Fired after every hit that did damage and did not kill. Damage is what was applied, NewHealth what is left. */
	UPROPERTY(BlueprintAssignable, Category = "VRT|Combat")
	FVRTDamagedSignature OnDamaged;

	/** Fired once, when health reaches 0. OnDamaged is not fired for the killing hit. */
	UPROPERTY(BlueprintAssignable, Category = "VRT|Combat")
	FVRTDeathSignature OnDeath;

	/** Health at full strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Combat", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:
	float Health = 100.f;
};
