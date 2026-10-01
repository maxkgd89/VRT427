#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "VRTGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVRTKeysChanged, int32, KeysCollected, int32, KeysTotal);

/** Level progress shared by gameplay and UI: keys collected and keys in the level. Level flow comes in step 8. */
UCLASS()
class VRT_API AVRTGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** A key announces itself at BeginPlay so the total is known without hard-coding it. */
	void RegisterKey();

	/** A key was picked up. */
	void AddKey();

	int32 GetKeysCollected() const { return KeysCollected; }
	int32 GetKeysTotal() const { return KeysTotal; }

	/** Fired whenever the collected or total key count changes. */
	UPROPERTY(BlueprintAssignable, Category = "VRT|GameFlow")
	FVRTKeysChanged OnKeysChanged;

private:
	int32 KeysCollected = 0;
	int32 KeysTotal = 0;
};
