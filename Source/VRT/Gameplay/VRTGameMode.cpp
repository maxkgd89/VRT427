#include "Gameplay/VRTGameMode.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Gameplay/VRTGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Level/VRTMazeBuilder.h"
#include "Player/VRTPawn.h"
#include "VRTLog.h"

AVRTGameMode::AVRTGameMode()
{
	DefaultPawnClass = AVRTPawn::StaticClass();
	GameStateClass = AVRTGameState::StaticClass();
}

void AVRTGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// The level number travels through map reloads as a URL option (?Level=N).
	LevelIndex = FMath::Max(1, UGameplayStatics::GetIntOption(Options, TEXT("Level"), 1));
	VRT_LOG(LogVRTGameFlow, Log, "InitGame map=%s level=%d", *MapName, LevelIndex);
}

void AVRTGameMode::StartPlay()
{
	Super::StartPlay();

	// The camera fade-in for the first level is started when the player logs in (HandleStartingNewPlayer).
	StartLevelIntro(false);
}

void AVRTGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (NewPlayer && NewPlayer->PlayerCameraManager)
	{
		NewPlayer->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeInSeconds, FLinearColor::Black, true, false);
	}
}

AActor* AVRTGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	for (TActorIterator<AVRTMazeBuilder> It(GetWorld()); It; ++It)
	{
		if (AActor* SpawnPoint = It->GetSpawnPoint())
		{
			VRT_LOG(LogVRTGameFlow, Log, "Spawning at the maze spawn point (%s), start requested by the editor/URL: '%s'", *SpawnPoint->GetName(), *IncomingName);
			return SpawnPoint;
		}
	}
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

void AVRTGameMode::StartLevelIntro(bool bFadeIn)
{
	if (AVRTGameState* State = GetGameState<AVRTGameState>())
	{
		State->SetLevelIndex(LevelIndex);
		State->SetLevelState(EVRTLevelState::Start);
	}

	if (bFadeIn)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeInSeconds, FLinearColor::Black, true, false);
			}
		}
	}

	// Start -> Playing once the fade from black is done.
	GetWorldTimerManager().SetTimer(FlowTimer, this, &AVRTGameMode::BeginPlaying, FMath::Max(FadeInSeconds, 0.01f), false);
}

void AVRTGameMode::BeginPlaying()
{
	if (AVRTGameState* State = GetGameState<AVRTGameState>())
	{
		State->SetLevelState(EVRTLevelState::Playing);
	}
}

void AVRTGameMode::CompleteLevel()
{
	const AVRTGameState* State = GetGameState<AVRTGameState>();
	if (!State || State->GetLevelState() != EVRTLevelState::Playing)
	{
		return;
	}

	VRT_LOG(LogVRTGameFlow, Log, "Level %d complete with %d / %d keys", LevelIndex, State->GetKeysCollected(), State->GetKeysTotal());
	GoToLevel(LevelIndex + 1);
}

void AVRTGameMode::RestartCurrentLevel()
{
	VRT_LOG(LogVRTGameFlow, Log, "Restarting level %d (same seed)", LevelIndex);
	GoToLevel(LevelIndex);
}

void AVRTGameMode::GoToLevel(int32 TargetLevel)
{
	AVRTGameState* State = GetGameState<AVRTGameState>();
	if (!State || State->GetLevelState() == EVRTLevelState::Complete)
	{
		return; // already leaving this level
	}

	PendingLevel = FMath::Max(1, TargetLevel);
	VRT_LOG(LogVRTGameFlow, Log, "Leaving level %d for level %d, fading out over %.1f s", LevelIndex, PendingLevel, FadeOutSeconds);
	State->SetLevelState(EVRTLevelState::Complete);

	// A fade hides the hitch of building the next maze, so there is no hard cut in the headset.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeOutSeconds, FLinearColor::Black, true, true);
		}
	}

	GetWorldTimerManager().SetTimer(FlowTimer, this, &AVRTGameMode::LoadPendingLevel, FMath::Max(FadeOutSeconds, 0.01f), false);
}

void AVRTGameMode::LoadPendingLevel()
{
	AVRTMazeBuilder* Builder = nullptr;
	for (TActorIterator<AVRTMazeBuilder> It(GetWorld()); It; ++It)
	{
		Builder = *It;
		break;
	}

	if (!Builder)
	{
		// Hand-made test level: reload the same map.
		const FString MapPath = UWorld::RemovePIEPrefix(GetOutermost()->GetName());
		VRT_LOG(LogVRTGameFlow, Log, "No maze builder in this level, reloading the map for level %d: map=%s", PendingLevel, *MapPath);
		UGameplayStatics::OpenLevel(this, FName(*MapPath), true, FString::Printf(TEXT("Level=%d"), PendingLevel));
		return;
	}

	// Maze level: regenerate in place. The builder replaces the geometry, keys, beacons and exit and resets
	// the key count; then the player moves to the new spawn.
	LevelIndex = PendingLevel;
	VRT_LOG(LogVRTGameFlow, Log, "Building level %d in place", LevelIndex);
	Builder->GenerateAndBuild(LevelIndex);
	if (AVRTPawn* Pawn = Cast<AVRTPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Builder->PlacePawnAtSpawn(Pawn);
		Pawn->NotifyTeleported();
	}

	StartLevelIntro(true);
}
