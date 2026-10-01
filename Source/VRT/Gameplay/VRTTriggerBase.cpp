#include "Gameplay/VRTTriggerBase.h"
#include "Components/SphereComponent.h"
#include "Debug/VRTDebugSettings.h"
#include "DrawDebugHelpers.h"
#include "Player/VRTPawn.h"
#include "VRTCollision.h"
#include "VRTLog.h"

AVRTTriggerBase::AVRTTriggerBase()
{
	// The tick only draws the debug sphere. Shipping builds start with it off (subclasses that need a tick turn it on).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = !UE_BUILD_SHIPPING;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(TriggerRadius);
	Trigger->SetCollisionProfileName(VRTCollision::TriggerProfile);
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);
}

void AVRTTriggerBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Trigger->SetSphereRadius(TriggerRadius);
}

void AVRTTriggerBase::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AVRTTriggerBase::HandleBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &AVRTTriggerBase::HandleEndOverlap);
}

void AVRTTriggerBase::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only the player's pawn counts; the overlap profile already excludes everything else.
	if (AVRTPawn* Pawn = Cast<AVRTPawn>(OtherActor))
	{
		VRT_LOG(LogVRTGameFlow, Verbose, "%s: player entered", *GetName());
		OnPlayerEntered(Pawn);
	}
}

void AVRTTriggerBase::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (AVRTPawn* Pawn = Cast<AVRTPawn>(OtherActor))
	{
		VRT_LOG(LogVRTGameFlow, Verbose, "%s: player left", *GetName());
		OnPlayerExited(Pawn);
	}
}

void AVRTTriggerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bShowDebugSphere || VRTDebug::ShowTriggers())
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Trigger->GetScaledSphereRadius(), 16, DebugColor, false, -1.f, 0, 0.5f);
	}
}
