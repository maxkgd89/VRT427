#include "Gameplay/VRTGameMode.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Gameplay/VRTGameState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
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

	// The level number travels through map changes as a URL option (?Level=N).
	LevelIndex = FMath::Max(1, UGameplayStatics::GetIntOption(Options, TEXT("Level"), 1));
	VRT_LOG(LogVRTGameFlow, Log, "InitGame map=%s level=%d", *MapName, LevelIndex);
}

void AVRTGameMode::StartPlay()
{
	Super::StartPlay();

	if (AVRTGameState* State = GetGameState<AVRTGameState>())
	{
		State->SetLevelIndex(LevelIndex);
		State->SetLevelState(EVRTLevelState::Start);
	}

	// Start -> Playing once the fade from black is done.
	GetWorldTimerManager().SetTimer(FlowTimer, this, &AVRTGameMode::BeginPlaying, FMath::Max(FadeInSeconds, 0.01f), false);
}

void AVRTGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (NewPlayer && NewPlayer->PlayerCameraManager)
	{
		NewPlayer->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeInSeconds, FLinearColor::Black, true, false);
	}
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
	AVRTGameState* State = GetGameState<AVRTGameState>();
	if (!State || State->GetLevelState() != EVRTLevelState::Playing)
	{
		return;
	}

	VRT_LOG(LogVRTGameFlow, Log, "Level %d complete with %d / %d keys, fading out over %.1f s", LevelIndex, State->GetKeysCollected(),
		State->GetKeysTotal(), FadeOutSeconds);
	State->SetLevelState(EVRTLevelState::Complete);

	// A fade hides the hitch of the map change, so there is no hard cut in the headset.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeOutSeconds, FLinearColor::Black, true, true);
		}
	}

	GetWorldTimerManager().SetTimer(FlowTimer, this, &AVRTGameMode::LoadNextLevel, FMath::Max(FadeOutSeconds, 0.01f), false);
}

void AVRTGameMode::LoadNextLevel()
{
	// The test level restarts the same map. From step 10 on this regenerates the labyrinth with a new seed.
	const FString MapPath = UWorld::RemovePIEPrefix(GetOutermost()->GetName());
	const FString Options = FString::Printf(TEXT("Level=%d"), LevelIndex + 1);
	VRT_LOG(LogVRTGameFlow, Log, "Loading next level %d: map=%s", LevelIndex + 1, *MapPath);
	UGameplayStatics::OpenLevel(this, FName(*MapPath), true, Options);
}
