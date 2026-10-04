#include "Combat/VRTHealthComponent.h"
#include "GameFramework/Controller.h"
#include "VRTLog.h"

UVRTHealthComponent::UVRTHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVRTHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UVRTHealthComponent::HandleTakeAnyDamage);
	}
}

void UVRTHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || !IsAlive())
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Max(0.f, Health - Damage);
	VRT_LOG(LogVRTCombat, Log, "%s: health %.0f -> %.0f of %.0f (damage %.0f from %s, instigator %s)", *GetNameSafe(DamagedActor), OldHealth, Health,
		MaxHealth, Damage, *GetNameSafe(DamageCauser), *GetNameSafe(InstigatedBy));

	if (Health <= 0.f)
	{
		VRT_LOG(LogVRTCombat, Log, "%s: died (killed by %s)", *GetNameSafe(DamagedActor), *GetNameSafe(DamageCauser));
		OnDeath.Broadcast(DamageCauser);
	}
	else
	{
		OnDamaged.Broadcast(Damage, Health, DamageCauser);
	}
}

void UVRTHealthComponent::ResetHealth()
{
	VRT_LOG(LogVRTCombat, Log, "%s: health %.0f -> %.0f (reset)", *GetNameSafe(GetOwner()), Health, MaxHealth);
	Health = MaxHealth;
}

void UVRTHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.f || !IsAlive())
	{
		return;
	}
	const float OldHealth = Health;
	Health = FMath::Min(MaxHealth, Health + Amount);
	VRT_LOG(LogVRTCombat, Log, "%s: health %.0f -> %.0f (healed %.0f)", *GetNameSafe(GetOwner()), OldHealth, Health, Amount);
}
