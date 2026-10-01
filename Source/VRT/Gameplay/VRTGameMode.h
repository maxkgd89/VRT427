#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "VRTGameMode.generated.h"

/**
 * Game mode that spawns AVRTPawn and runs the level flow: Start -> Playing -> Complete -> (fade) -> next level.
 * The test level restarts the same map; the level number travels in the "Level" URL option.
 */
UCLASS()
class VRT_API AVRTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVRTGameMode();

	/** Called by the level exit when the player qualifies. Fades out, then loads the next level. */
	void CompleteLevel();

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** Seconds of the fade from black when a level starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|GameFlow", meta = (ClampMin = "0.0"))
	float FadeInSeconds = 1.f;

	/** Seconds of the fade to black when a level is completed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|GameFlow", meta = (ClampMin = "0.0"))
	float FadeOutSeconds = 1.5f;

private:
	void BeginPlaying();
	void LoadNextLevel();

	int32 LevelIndex = 1;
	FTimerHandle FlowTimer;
};
