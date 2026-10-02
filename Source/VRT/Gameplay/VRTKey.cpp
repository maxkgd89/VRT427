#include "Gameplay/VRTKey.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Gameplay/VRTBeacon.h"
#include "Gameplay/VRTGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/VRTPawn.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTCollision.h"
#include "VRTLog.h"

AVRTKey::AVRTKey()
{
	DebugColor = FColor::Yellow;
	PrimaryActorTick.bStartWithTickEnabled = true; // the key spins
	BeaconClass = AVRTBeacon::StaticClass();

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	// Engine cube is 100 cm wide: a 12 cm gold cube, turned on a corner.
	Mesh->SetRelativeScale3D(FVector(0.12f));
	Mesh->SetRelativeRotation(FRotator(35.f, 45.f, 0.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMaterial(TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
	if (GoldMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, GoldMaterial.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> PickupCue(TEXT("/Game/StarterContent/Audio/Light02_Cue.Light02_Cue"));
	if (PickupCue.Succeeded())
	{
		PickupSound = PickupCue.Object;
	}
}

void AVRTKey::BeginPlay()
{
	Super::BeginPlay();

	if (AVRTGameState* GameState = GetWorld()->GetGameState<AVRTGameState>())
	{
		GameState->RegisterKey();
	}
	else
	{
		VRT_LOG(LogVRTGameFlow, Warning, "%s: no AVRTGameState, the key won't be counted (check the game mode)", *GetName());
	}

	// Stand the beacon on the floor beside the key.
	if (BeaconClass)
	{
		const FVector KeyLocation = GetActorLocation();
		FVector BeaconLocation(KeyLocation.X + BeaconOffset.X, KeyLocation.Y + BeaconOffset.Y, KeyLocation.Z);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(VRTBeaconFloorTrace), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, BeaconLocation, BeaconLocation - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
		{
			BeaconLocation.Z = Hit.ImpactPoint.Z;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Beacon = GetWorld()->SpawnActor<AVRTBeacon>(BeaconClass, BeaconLocation, FRotator::ZeroRotator, SpawnParams);
		VRT_LOG(LogVRTGameFlow, Log, "%s: key at(World)=%s, beacon %s at(World)=%s", *GetName(), *KeyLocation.ToCompactString(),
			*GetNameSafe(Beacon), *BeaconLocation.ToCompactString());
	}
}

void AVRTKey::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The beacon belongs to this key: without this, destroying a key (maze rebuilt while playing) leaves a false beam behind.
	if (Beacon)
	{
		Beacon->Destroy();
		Beacon = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AVRTKey::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bCollected)
	{
		Mesh->AddRelativeRotation(FRotator(0.f, SpinSpeed * DeltaSeconds, 0.f));
	}
}

void AVRTKey::OnPlayerEntered(AVRTPawn* Pawn)
{
	if (bCollected)
	{
		return;
	}

	bCollected = true;
	VRT_LOG(LogVRTGameFlow, Log, "%s: collected by %s", *GetName(), *GetNameSafe(Pawn));

	if (AVRTGameState* GameState = GetWorld()->GetGameState<AVRTGameState>())
	{
		GameState->AddKey();
	}

	Pawn->PlayHapticPulseBothHands(PickupHapticIntensity, 0.2f);

	if (PickupSound)
	{
		UAudioComponent* Audio = UGameplayStatics::SpawnSoundAtLocation(this, PickupSound, GetActorLocation());
		if (Audio && (PickupSound->IsLooping() || PickupSound->GetDuration() > PickupSoundMaxSeconds))
		{
			FTimerHandle CutOffHandle;
			TWeakObjectPtr<UAudioComponent> WeakAudio(Audio);
			GetWorldTimerManager().SetTimer(CutOffHandle, FTimerDelegate::CreateLambda([WeakAudio]()
			{
				if (WeakAudio.IsValid())
				{
					WeakAudio->FadeOut(0.05f, 0.f);
				}
			}), PickupSoundMaxSeconds, false);
		}
	}

	// Hide the key and switch the trigger off; the actor stays so the level can be reset later.
	SetActorHiddenInGame(true);
	Trigger->SetGenerateOverlapEvents(false);

	if (Beacon)
	{
		Beacon->FadeOut(BeaconFadeSeconds);
	}
}
