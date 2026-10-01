#include "Player/VRTHolsterComponent.h"
#include "Debug/VRTDebugSettings.h"
#include "DrawDebugHelpers.h"
#include "Player/VRTHandComponent.h"
#include "Engine/World.h"
#include "VRTLog.h"
#include "Weapons/VRTWeaponBase.h"

UVRTHolsterComponent::UVRTHolsterComponent()
{
	// Debug drawing is the only thing this tick does, so Shipping builds skip it entirely.
	PrimaryComponentTick.bCanEverTick = !UE_BUILD_SHIPPING;
	PrimaryComponentTick.bStartWithTickEnabled = !UE_BUILD_SHIPPING;

	// Only the right hand draws from holsters.
	AllowedHand = EVRTHandFilter::RightOnly;
	GrabRadius = 18.f;
	InitSphereRadius(GrabRadius);
}

void UVRTHolsterComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UVRTHolsterComponent::HandleHandEnter);
	OnComponentEndOverlap.AddDynamic(this, &UVRTHolsterComponent::HandleHandExit);
	OnPointGrabbed.AddDynamic(this, &UVRTHolsterComponent::HandleDrawn);
	OnPointReleased.AddDynamic(this, &UVRTHolsterComponent::HandleReleased);

	if (WeaponClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Weapon = GetWorld()->SpawnActor<AVRTWeaponBase>(WeaponClass, GetComponentTransform(), Params);
		if (Weapon)
		{
			Weapon->SetHolster(this);
		}
		else
		{
			VRT_LOG(LogVRTHolster, Error, "%s: could not spawn weapon %s", *PointId.ToString(), *GetNameSafe(WeaponClass));
		}
	}
}

void UVRTHolsterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Weapon)
	{
		Weapon->Destroy();
		Weapon = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UVRTHolsterComponent::UpdateForHeadHeight(float HeadHeight)
{
	SetRelativeLocation(FVector(ForwardOffset, RightOffset, HeightFraction * HeadHeight + HeightOffset));
}

void UVRTHolsterComponent::HandleHandEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (const UVRTHandComponent* HandComp = Cast<UVRTHandComponent>(OtherComp))
	{
		VRT_LOG(LogVRTHolster, Log, "%s: hand %s entered zone (allowed=%d, location(World)=%s)", *PointId.ToString(),
			*UEnum::GetValueAsString(HandComp->Hand), CanBeGrabbedBy(HandComp) ? 1 : 0, *GetComponentLocation().ToCompactString());
	}
}

void UVRTHolsterComponent::HandleHandExit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (const UVRTHandComponent* HandComp = Cast<UVRTHandComponent>(OtherComp))
	{
		VRT_LOG(LogVRTHolster, Log, "%s: hand %s left zone", *PointId.ToString(), *UEnum::GetValueAsString(HandComp->Hand));
	}
}

void UVRTHolsterComponent::HandleDrawn(UVRTGrabPointComponent* /*GrabPoint*/, UVRTHandComponent* Hand)
{
	VRT_LOG(LogVRTHolster, Log, "%s: Holstered -> Drawn by %s (grip), weapon=%s", *PointId.ToString(), *UEnum::GetValueAsString(Hand->Hand),
		*GetNameSafe(Weapon));
	if (Weapon)
	{
		Weapon->AttachToHand(Hand);
	}
}

void UVRTHolsterComponent::HandleReleased(UVRTGrabPointComponent* /*GrabPoint*/, UVRTHandComponent* Hand)
{
	VRT_LOG(LogVRTHolster, Log, "%s: Drawn -> Holstered (grip released)", *PointId.ToString());
	if (Weapon)
	{
		Weapon->ReturnToHolster();
	}
}

void UVRTHolsterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!VRTDebug::ShowHolsters())
	{
		return;
	}

	// Cyan: zone ready. Red: drawn (held by the right hand).
	DrawDebugSphere(GetWorld(), GetComponentLocation(), GetScaledSphereRadius(), 16, IsHeld() ? FColor::Red : FColor::Cyan, false, -1.f, 0, 0.3f);
}
