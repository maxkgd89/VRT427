#include "Player/VRTPlayerPawnBase.h"
#include "Combat/VRTHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Gameplay/VRTGameMode.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "VRTCollision.h"
#include "VRTLog.h"

AVRTPlayerPawnBase::AVRTPlayerPawnBase()
{
	// PlayerStart may sit in or on the floor; BeginPlay of the pawn puts it on the floor afterwards.
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	Capsule->SetCollisionProfileName(VRTCollision::PlayerProfile);
	Capsule->SetCanEverAffectNavigation(false); // the player must not carve a hole into the navigation mesh
	SetRootComponent(Capsule);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = WalkSpeed;
	Movement->Acceleration = 2000.f;
	Movement->Deceleration = 2000.f;

	Health = CreateDefaultSubobject<UVRTHealthComponent>(TEXT("Health"));
}

void AVRTPlayerPawnBase::BeginPlay()
{
	Super::BeginPlay();

	Health->OnDamaged.AddDynamic(this, &AVRTPlayerPawnBase::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &AVRTPlayerPawnBase::HandleDeath);
}

FVector AVRTPlayerPawnBase::GetFacingDirectionFlat() const
{
	return GetActorForwardVector().GetSafeNormal2D();
}

void AVRTPlayerPawnBase::NotifyTeleported()
{
	// A new level (or a restart after dying) starts with full health.
	bDead = false;
	Health->ResetHealth();
}

void AVRTPlayerPawnBase::HandleDamaged(float Damage, float NewHealth, AActor* DamageCauser)
{
	VRT_LOG(LogVRTCombat, Log, "Player hit for %.0f, %.0f of %.0f health left", Damage, NewHealth, Health->GetMaxHealth());
	OnDamagedFeedback(Damage, NewHealth, DamageCauser);
}

void AVRTPlayerPawnBase::HandleDeath(AActor* DamageCauser)
{
	bDead = true;
	VRT_LOG(LogVRTCombat, Log, "Player died (killed by %s), restarting the level", *GetNameSafe(DamageCauser));
	OnDied(DamageCauser);

	if (AVRTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AVRTGameMode>() : nullptr)
	{
		GameMode->RestartCurrentLevel();
	}
}
