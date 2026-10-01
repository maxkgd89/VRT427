#include "Player/VRTHandComponent.h"
#include "Debug/VRTDebugSettings.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/VRTGrabPointComponent.h"
#include "VRTCollision.h"
#include "VRTLog.h"

UVRTHandComponent::UVRTHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SetCollisionProfileName(VRTCollision::HandProfile);
	SetGenerateOverlapEvents(true);
	SetHiddenInGame(true);
	InitSphereRadius(GrabRadius);
}

void UVRTHandComponent::OnRegister()
{
	Super::OnRegister();
	SetSphereRadius(GrabRadius);
}

void UVRTHandComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UVRTHandComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UVRTHandComponent::HandleEndOverlap);

	// Grab points that already overlap at spawn don't fire BeginOverlap.
	TArray<UPrimitiveComponent*> Overlapping;
	GetOverlappingComponents(Overlapping);
	for (UPrimitiveComponent* Comp : Overlapping)
	{
		if (UVRTGrabPointComponent* Point = Cast<UVRTGrabPointComponent>(Comp))
		{
			CandidatesInRange.AddUnique(Point);
		}
	}
}

void UVRTHandComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UVRTGrabPointComponent* Point = Cast<UVRTGrabPointComponent>(OtherComp);
	if (!Point)
	{
		return;
	}

	CandidatesInRange.AddUnique(Point);
	const bool bGrabbable = Point->CanBeGrabbedBy(this);
	VRT_LOG(LogVRTHand, Verbose, "%s: in range of %s on %s (grabbable=%d, candidates=%d)", *UEnum::GetValueAsString(Hand),
		*Point->PointId.ToString(), *GetNameSafe(Point->GetOwner()), bGrabbable ? 1 : 0, CandidatesInRange.Num());

	if (bGrabbable && !IsHolding())
	{
		PlayHapticPulse(0.3f, 0.05f);
	}
}

void UVRTHandComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (UVRTGrabPointComponent* Point = Cast<UVRTGrabPointComponent>(OtherComp))
	{
		CandidatesInRange.Remove(Point);
		VRT_LOG(LogVRTHand, Verbose, "%s: out of range of %s on %s (candidates=%d)", *UEnum::GetValueAsString(Hand),
			*Point->PointId.ToString(), *GetNameSafe(Point->GetOwner()), CandidatesInRange.Num());
	}
}

UVRTGrabPointComponent* UVRTHandComponent::GetBestCandidate() const
{
	UVRTGrabPointComponent* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector HandLocation = GetComponentLocation();

	for (UVRTGrabPointComponent* Point : CandidatesInRange)
	{
		if (!Point || !Point->CanBeGrabbedBy(this))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(HandLocation, Point->GetComponentLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Point;
		}
	}
	return Best;
}

void UVRTHandComponent::OnGripPressed()
{
	VRT_LOG(LogVRTHand, Log, "%s: grip pressed (candidates=%d, holding=%d)", *UEnum::GetValueAsString(Hand),
		CandidatesInRange.Num(), IsHolding() ? 1 : 0);

	if (IsHolding())
	{
		return;
	}

	UVRTGrabPointComponent* Target = GetBestCandidate();
	if (!Target)
	{
		VRT_LOG(LogVRTHand, Verbose, "%s: nothing to grab", *UEnum::GetValueAsString(Hand));
		return;
	}

	HeldPoint = Target;
	VRT_LOG(LogVRTHand, Log, "%s: held None -> %s on %s (dist(World)=%.1f cm)", *UEnum::GetValueAsString(Hand),
		*Target->PointId.ToString(), *GetNameSafe(Target->GetOwner()),
		FVector::Dist(GetComponentLocation(), Target->GetComponentLocation()));
	Target->NotifyGrabbed(this);
	PlayHapticPulse(0.6f, 0.1f);
}

void UVRTHandComponent::OnGripReleased()
{
	VRT_LOG(LogVRTHand, Verbose, "%s: grip released (holding=%d)", *UEnum::GetValueAsString(Hand), IsHolding() ? 1 : 0);

	if (!HeldPoint)
	{
		return;
	}

	UVRTGrabPointComponent* Released = HeldPoint;
	HeldPoint = nullptr;
	VRT_LOG(LogVRTHand, Log, "%s: held %s on %s -> None (grip released)", *UEnum::GetValueAsString(Hand),
		*Released->PointId.ToString(), *GetNameSafe(Released->GetOwner()));
	Released->NotifyReleased(this);
}

void UVRTHandComponent::PlayHapticPulse(float Intensity, float DurationSec)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	PC->SetHapticsByValue(1.f, FMath::Clamp(Intensity, 0.f, 1.f), Hand);
	GetWorld()->GetTimerManager().SetTimer(HapticTimer, this, &UVRTHandComponent::StopHaptics, FMath::Max(DurationSec, 0.01f), false);
}

void UVRTHandComponent::StopHaptics()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
	{
		PC->SetHapticsByValue(0.f, 0.f, Hand);
	}
}

void UVRTHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!VRTDebug::ShowGrab())
	{
		return;
	}

	// Green: nothing in range. Yellow: a grabbable point in range. Red: holding something.
	const FColor Color = IsHolding() ? FColor::Red : (GetBestCandidate() ? FColor::Yellow : FColor::Green);
	DrawDebugSphere(GetWorld(), GetComponentLocation(), GrabRadius, 12, Color, false, -1.f, 0, 0.3f);
}
