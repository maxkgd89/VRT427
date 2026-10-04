#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "VRTGameMode.generated.h"

/**
 * Game mode that spawns AVRTPawn and runs the level flow: Start -> Playing -> Complete -> (fade) -> next level.
 *
 * With an AVRTMazeBuilder in the level, the next level is built in place behind a fade: the builder generates a
 * bigger maze with seed + 1 and the player is moved to the new spawn. Without a builder (the hand-made test map)
 * the map is reloaded and the level number travels in the "Level" URL option.
 */
UCLASS()
class VRT_API AVRTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVRTGameMode();

	/** Called by the level exit when the player qualifies. Fades out, then loads the next level. */
	void CompleteLevel();

	/** Fades out and goes to the given level (1-based). Also used for restarts: pass the current level. */
	void GoToLevel(int32 TargetLevel);

	/** The player died: fades out and rebuilds the current level (same seed and size). */
	void RestartCurrentLevel();

	int32 GetLevelIndex() const { return LevelIndex; }

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/**
	 * Uses the maze builder's spawn point when there is one. The builder creates its PlayerStart while the game starts, so
	 * the editor does not know it exists and "Play" would otherwise start at the viewport camera (possibly on top of a wall).
	 */
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	/** Seconds of the fade from black when a level starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|GameFlow", meta = (ClampMin = "0.0"))
	float FadeInSeconds = 1.f;

	/** Seconds of the fade to black when a level ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|GameFlow", meta = (ClampMin = "0.0"))
	float FadeOutSeconds = 1.5f;

private:
	/** Start state, fade in, then Playing. */
	void StartLevelIntro(bool bFadeIn);
	void BeginPlaying();

	/** Runs after the fade out: rebuilds the maze in place, or reloads the map if there is no builder. */
	void LoadPendingLevel();

	int32 LevelIndex = 1;
	int32 PendingLevel = 1;
	FTimerHandle FlowTimer;
};
