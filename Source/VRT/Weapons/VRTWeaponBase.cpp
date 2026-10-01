#include "Weapons/VRTWeaponBase.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Player/VRTHandComponent.h"
#include "Player/VRTHolsterComponent.h"
#include "VRTCollision.h"
#include "VRTLog.h"
#include "Weapons/VRTProjectile.h"

AVRTWeaponBase::AVRTWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(VRTCollision::WeaponProfile);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCastShadow(false);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Root);

	ProjectileClass = AVRTProjectile::StaticClass();
}

void AVRTWeaponBase::SetHolster(UVRTHolsterComponent* InHolster)
{
	Holster = InHolster;
	ReturnToHolster();
}

void AVRTWeaponBase::AttachToHand(UVRTHandComponent* Hand)
{
	if (!Hand)
	{
		return;
	}

	HoldingHand = Hand;
	AttachToComponent(Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(GripLocation);
	SetActorRelativeRotation(GripRotation);
	VRT_LOG(LogVRTWeapon, Log, "%s: Holstered -> Held by %s (grip offset(Local)=%s)", *GetName(),
		*UEnum::GetValueAsString(Hand->Hand), *GripLocation.ToCompactString());
}

void AVRTWeaponBase::ReturnToHolster()
{
	const bool bWasHeld = IsHeld();
	HoldingHand = nullptr;

	// Detach -> hide -> move into the holster -> show. The weapon is never destroyed.
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorHiddenInGame(true);
	if (Holster)
	{
		AttachToComponent(Holster, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(HolsterLocation);
		SetActorRelativeRotation(HolsterRotation);
	}
	SetActorHiddenInGame(false);

	VRT_LOG(LogVRTWeapon, Log, "%s: %s -> Holstered (holster=%s)", *GetName(), bWasHeld ? TEXT("Held") : TEXT("Spawned"),
		Holster ? *Holster->PointId.ToString() : TEXT("none"));
}

void AVRTWeaponBase::OnTriggerPressed()
{
	TryFire();
}

void AVRTWeaponBase::OnTriggerReleased()
{
}

bool AVRTWeaponBase::TryFire()
{
	if (!IsHeld())
	{
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float SinceLast = Now - LastFireTime;
	if (SinceLast < FireInterval)
	{
		VRT_LOG(LogVRTWeapon, Verbose, "%s: cooldown, %.2f s left", *GetName(), FireInterval - SinceLast);
		return false;
	}

	LastFireTime = Now;
	Fire();
	return true;
}

void AVRTWeaponBase::Fire()
{
	const FTransform MuzzleTransform = Muzzle->GetComponentTransform();
	VRT_LOG(LogVRTWeapon, Log, "%s: fire, muzzle(World)=%s dir(World)=%s", *GetName(),
		*MuzzleTransform.GetLocation().ToCompactString(), *MuzzleTransform.GetRotation().GetForwardVector().ToCompactString());

	if (ProjectileClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = GetInstigator();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AVRTProjectile>(ProjectileClass, MuzzleTransform.GetLocation(), MuzzleTransform.GetRotation().Rotator(), Params);
	}

	if (FireSound)
	{
		UAudioComponent* Audio = UGameplayStatics::SpawnSoundAtLocation(this, FireSound, MuzzleTransform.GetLocation());
		const bool bLooping = FireSound->IsLooping();
		VRT_LOG(LogVRTWeapon, Verbose, "%s: fire sound %s looping=%d duration=%.2f s", *GetName(), *GetNameSafe(FireSound), bLooping ? 1 : 0, FireSound->GetDuration());

		// A looping cue would otherwise play forever at the spot where it was fired.
		if (Audio && (bLooping || FireSound->GetDuration() > FireSoundMaxSeconds))
		{
			FTimerHandle CutOffHandle;
			TWeakObjectPtr<UAudioComponent> WeakAudio(Audio);
			GetWorldTimerManager().SetTimer(CutOffHandle, FTimerDelegate::CreateLambda([WeakAudio]()
			{
				if (WeakAudio.IsValid())
				{
					WeakAudio->FadeOut(0.05f, 0.f);
				}
			}), FireSoundMaxSeconds, false);
		}
	}

	if (HoldingHand)
	{
		HoldingHand->PlayHapticPulse(FireHapticIntensity, 0.1f);
	}
}
