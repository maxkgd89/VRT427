#pragma once

#include "CoreMinimal.h"
#include "Gameplay/VRTTriggerBase.h"
#include "VRTLevelExit.generated.h"

class AVRTBeacon;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * The exit, at the spawn point. Walking back into it with enough keys completes the level. Until then the pad
 * is red; with enough keys it turns green. Entering while short on keys gives a haptic buzz.
 * The exit only reacts after the player has left it once, so spawning inside it does nothing.
 */
UCLASS()
class VRT_API AVRTLevelExit : public AVRTTriggerBase
{
	GENERATED_BODY()

public:
	AVRTLevelExit();

	/** Keys needed to leave. Call before the exit begins play. */
	void SetRequiredKeys(int32 InRequiredKeys) { RequiredKeys = InRequiredKeys; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPlayerEntered(AVRTPawn* Pawn) override;
	virtual void OnPlayerExited(AVRTPawn* Pawn) override;

	UFUNCTION()
	void RefreshVisual();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Exit")
	UStaticMeshComponent* Pad;

	/** Keys needed to leave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Exit", meta = (ClampMin = "0"))
	int32 RequiredKeys = 2;

	/** Pad colour while the exit is closed / open (linear, values above 1 glow). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Exit")
	FLinearColor ClosedColor = FLinearColor(3.f, 0.1f, 0.05f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Exit")
	FLinearColor OpenColor = FLinearColor(0.1f, 3.f, 0.3f, 1.f);

	/** Colour of the beacon that marks the open exit (linear, values above 1 glow). Yellow, unlike the blue key beacons. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Exit")
	FLinearColor BeaconColor = FLinearColor(4.f, 3.f, 0.1f, 1.f);

	/** Buzz on both hands when the player enters without enough keys (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Exit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeniedHapticIntensity = 0.6f;

	/** Distance from the actor origin down to the floor, cm. The pad sits on the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Exit")
	float FloorOffset = 85.f;

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PadMaterial = nullptr;

	/** Marks the exit from far away. Hidden until the player holds enough keys. */
	UPROPERTY(Transient)
	AVRTBeacon* Beacon = nullptr;

	/** False until the player has left the trigger once. */
	bool bArmed = false;
};
