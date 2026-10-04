// Console commands for testing health and damage (step 11). Output goes to LogVRTCombat.
//   VRT.Player.Damage [amount]   hurts the player through the normal damage flow (default 25)
//   VRT.Player.Heal [amount]     heals the player (default: to full)
//   VRT.Player.Health            logs the player health
//   VRT.Dummy.Spawn [health]     spawns a shooting target with health 3 m in front of the player
#include "CoreMinimal.h"
#include "Combat/VRTHealthComponent.h"
#include "Combat/VRTTargetDummy.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/VRTPlayerPawnBase.h"
#include "VRTLog.h"

namespace
{
	AVRTPlayerPawnBase* GetPlayerPawn(UWorld* World)
	{
		return World ? Cast<AVRTPlayerPawnBase>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GPlayerDamage(
		TEXT("VRT.Player.Damage"),
		TEXT("Hurts the player. Args: [amount, default 25]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AVRTPlayerPawnBase* Pawn = GetPlayerPawn(World))
			{
				const float Amount = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 25.f;
				UGameplayStatics::ApplyDamage(Pawn, Amount, nullptr, nullptr, nullptr);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GPlayerHeal(
		TEXT("VRT.Player.Heal"),
		TEXT("Heals the player. Args: [amount, default full health]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AVRTPlayerPawnBase* Pawn = GetPlayerPawn(World))
			{
				UVRTHealthComponent* Health = Pawn->GetHealthComponent();
				Health->Heal(Args.Num() > 0 ? FCString::Atof(*Args[0]) : Health->GetMaxHealth());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GPlayerHealth(
		TEXT("VRT.Player.Health"),
		TEXT("Logs the player health (LogVRTCombat)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (AVRTPlayerPawnBase* Pawn = GetPlayerPawn(World))
			{
				const UVRTHealthComponent* Health = Pawn->GetHealthComponent();
				VRT_LOG(LogVRTCombat, Log, "Player health %.0f / %.0f, alive=%d", Health->GetHealth(), Health->GetMaxHealth(), Health->IsAlive() ? 1 : 0);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GDummySpawn(
		TEXT("VRT.Dummy.Spawn"),
		TEXT("Spawns a shooting target with health 3 m in front of the player, on the floor, turned toward the player. Args: [health, default 100]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const AVRTPlayerPawnBase* Pawn = GetPlayerPawn(World);
			if (!Pawn)
			{
				return;
			}

			// 3 m in front of the player (flattened), standing on the floor.
			const FVector Forward = Pawn->GetFacingDirectionFlat();
			const FVector SpawnWorld = Pawn->GetActorLocation() - FVector(0.f, 0.f, Pawn->GetCapsuleHalfHeight()) + Forward * 300.f + FVector(0.f, 0.f, 90.f);
			const FTransform SpawnTransform(FRotator(0.f, (-Forward).Rotation().Yaw, 0.f), SpawnWorld);

			AVRTTargetDummy* Dummy = World->SpawnActorDeferred<AVRTTargetDummy>(AVRTTargetDummy::StaticClass(), SpawnTransform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Dummy)
			{
				if (Args.Num() > 0)
				{
					Dummy->GetHealthComponent()->MaxHealth = FMath::Max(1.f, FCString::Atof(*Args[0]));
				}
				Dummy->FinishSpawning(SpawnTransform);
				VRT_LOG(LogVRTCombat, Log, "Spawned target dummy at(World)=%s", *SpawnWorld.ToCompactString());
			}
		}));
}
