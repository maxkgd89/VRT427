#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "VRTGameState.generated.h"

/** Where the current level is in its life: Start -> Playing -> Complete. */
UENUM(BlueprintType)
enum class EVRTLevelState : uint8
{
	Start,
	Playing,
	Complete
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVRTKeysChanged, int32, KeysCollected, int32, KeysTotal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVRTProgressChanged);

/** Level progress shared by gameplay and UI: keys, the exit requirement, level number and level state. */
UCLASS()
class VRT_API AVRTGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** A key announces itself at BeginPlay so the total is known without hard-coding it. */
	void RegisterKey();

	/** A key was picked up. */
	void AddKey();

	/** The level exit announces how many keys it needs. */
	void SetRequiredKeys(int32 InRequiredKeys);

	void SetLevelIndex(int32 InLevelIndex);
	void SetLevelState(EVRTLevelState NewState);

	int32 GetKeysCollected() const { return KeysCollected; }
	int32 GetKeysTotal() const { return KeysTotal; }
	int32 GetRequiredKeys() const { return RequiredKeys; }
	int32 GetLevelIndex() const { return LevelIndex; }
	EVRTLevelState GetLevelState() const { return LevelState; }

	/** True once enough keys are collected for the exit to open. */
	bool HasEnoughKeys() const { return KeysCollected >= RequiredKeys; }

	/** Fired whenever the collected or total key count changes. */
	UPROPERTY(BlueprintAssignable, Category = "VRT|GameFlow")
	FVRTKeysChanged OnKeysChanged;

	/** Fired on any change the wrist display or the exit cares about (keys, requirement, level, state). */
	UPROPERTY(BlueprintAssignable, Category = "VRT|GameFlow")
	FVRTProgressChanged OnProgressChanged;

private:
	int32 KeysCollected = 0;
	int32 KeysTotal = 0;
	int32 RequiredKeys = 2;
	int32 LevelIndex = 1;
	EVRTLevelState LevelState = EVRTLevelState::Start;
};
