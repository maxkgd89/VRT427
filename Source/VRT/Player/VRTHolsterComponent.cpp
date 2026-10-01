#include "Player/VRTHolsterComponent.h"
#include "Debug/VRTDebugSettings.h"
#include "DrawDebugHelpers.h"
#include "Player/VRTHandComponent.h"
#include "VRTLog.h"

UVRTHolsterComponent::UVRTHolsterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

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
	VRT_LOG(LogVRTHolster, Log, "%s: Holstered -> Drawn by %s (grip)", *PointId.ToString(), *UEnum::GetValueAsString(Hand->Hand));
}

void UVRTHolsterComponent::HandleReleased(UVRTGrabPointComponent* /*GrabPoint*/, UVRTHandComponent* Hand)
{
	VRT_LOG(LogVRTHolster, Log, "%s: Drawn -> Holstered (grip released)", *PointId.ToString());
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
