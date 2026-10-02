#include "Gameplay/VRTLevelExit.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Gameplay/VRTBeacon.h"
#include "Gameplay/VRTGameMode.h"
#include "Gameplay/VRTGameState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/VRTPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTLog.h"

namespace
{
	// Same unlit emissive material as the beacons (created by Tools/Editor/create_test_level.py).
	const TCHAR* PadMaterialPath = TEXT("/Game/VRT/Materials/M_Beacon.M_Beacon");
	const TCHAR* FallbackMaterialPath = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");
}

AVRTLevelExit::AVRTLevelExit()
{
	DebugColor = FColor::Green;
	TriggerRadius = 100.f;

	Pad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pad"));
	Pad->SetupAttachment(Trigger);
	Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pad->SetCastShadow(false);
	// Engine cylinder is 100 x 100 cm: a flat disc, 160 cm across and 4 cm thick, lying on the floor.
	Pad->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.04f));
	Pad->SetRelativeLocation(FVector(0.f, 0.f, -FloorOffset + 2.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Pad->SetStaticMesh(CylinderMesh.Object);
	}
}

void AVRTLevelExit::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, PadMaterialPath, nullptr, LOAD_NoWarn);
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, FallbackMaterialPath);
	}
	if (Base)
	{
		PadMaterial = UMaterialInstanceDynamic::Create(Base, this);
		Pad->SetMaterial(0, PadMaterial);
	}

	// Yellow beacon on the floor under the exit. It only shows once the exit is open (see RefreshVisual).
	FActorSpawnParameters BeaconParams;
	BeaconParams.Owner = this;
	BeaconParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	BeaconParams.bDeferConstruction = true;
	const FTransform BeaconTransform(GetActorLocation() - FVector(0.f, 0.f, FloorOffset));
	Beacon = GetWorld()->SpawnActor<AVRTBeacon>(AVRTBeacon::StaticClass(), BeaconTransform, BeaconParams);
	if (Beacon)
	{
		Beacon->SetColor(BeaconColor);
		Beacon->FinishSpawning(BeaconTransform);
		Beacon->SetActive(false);
	}

	if (AVRTGameState* State = GetWorld()->GetGameState<AVRTGameState>())
	{
		State->SetRequiredKeys(RequiredKeys);
		State->OnProgressChanged.AddDynamic(this, &AVRTLevelExit::RefreshVisual);
	}
	RefreshVisual();
}

void AVRTLevelExit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Beacon)
	{
		Beacon->Destroy();
		Beacon = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AVRTLevelExit::RefreshVisual()
{
	const AVRTGameState* State = GetWorld()->GetGameState<AVRTGameState>();
	const bool bOpen = State && State->HasEnoughKeys();
	if (PadMaterial)
	{
		PadMaterial->SetVectorParameterValue(TEXT("Color"), bOpen ? OpenColor : ClosedColor);
	}
	if (Beacon)
	{
		if (Beacon->IsHidden() == bOpen)
		{
			VRT_LOG(LogVRTGameFlow, Log, "Exit beacon %s (keys %d, need %d)", bOpen ? TEXT("on") : TEXT("off"), State ? State->GetKeysCollected() : 0, RequiredKeys);
		}
		Beacon->SetActive(bOpen);
	}
}

void AVRTLevelExit::OnPlayerEntered(AVRTPawn* Pawn)
{
	AVRTGameState* State = GetWorld()->GetGameState<AVRTGameState>();
	if (!State || State->GetLevelState() != EVRTLevelState::Playing)
	{
		return;
	}

	if (!bArmed)
	{
		VRT_LOG(LogVRTGameFlow, Verbose, "Exit entered before the player left it once, ignored (keys %d / %d)", State->GetKeysCollected(), RequiredKeys);
		return;
	}

	if (State->HasEnoughKeys())
	{
		VRT_LOG(LogVRTGameFlow, Log, "Exit check passed: %d keys (need %d)", State->GetKeysCollected(), RequiredKeys);
		if (AVRTGameMode* GameMode = GetWorld()->GetAuthGameMode<AVRTGameMode>())
		{
			GameMode->CompleteLevel();
		}
	}
	else
	{
		const int32 Missing = RequiredKeys - State->GetKeysCollected();
		VRT_LOG(LogVRTGameFlow, Log, "Exit check failed: %d keys, need %d more", State->GetKeysCollected(), Missing);
		Pawn->PlayHapticPulseBothHands(DeniedHapticIntensity, 0.4f);
	}
}

void AVRTLevelExit::OnPlayerExited(AVRTPawn* Pawn)
{
	bArmed = true;
}
