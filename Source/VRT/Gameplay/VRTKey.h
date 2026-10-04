#pragma once

#include "CoreMinimal.h"
#include "Gameplay/VRTTriggerBase.h"
#include "VRTKey.generated.h"

class AVRTBeacon;
class USoundBase;
class UStaticMeshComponent;

/**
 * A collectable key. The player picks it up by walking into its invisible trigger. It then counts in the
 * game state, hides, and fades its beacon out.
 */
UCLASS()
class VRT_API AVRTKey : public AVRTTriggerBase
{
	GENERATED_BODY()

public:
	AVRTKey();

	bool IsCollected() const { return bCollected; }

	/** Where the beacon stands relative to the key, horizontal, cm (World axes). Call before the key begins play. */
	void SetBeaconOffset(const FVector2D& InOffset) { BeaconOffset = InOffset; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPlayerEntered(AVRTPlayerPawnBase* Pawn) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Key")
	UStaticMeshComponent* Mesh;

	/** Beacon spawned beside the key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Key")
	TSubclassOf<AVRTBeacon> BeaconClass;

	/** Where the beacon stands relative to the key, horizontal, cm (World axes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Key")
	FVector2D BeaconOffset = FVector2D(60.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Key")
	USoundBase* PickupSound = nullptr;

	/** A pickup sound that loops or is longer than this is cut off after this many seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Key", meta = (ClampMin = "0.1"))
	float PickupSoundMaxSeconds = 1.f;

	/** Haptic pulse on both hands at pickup (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Key", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PickupHapticIntensity = 0.7f;

	/** Seconds the beacon takes to fade out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Key", meta = (ClampMin = "0.0"))
	float BeaconFadeSeconds = 1.f;

	/** Spin speed of the key, degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Key")
	float SpinSpeed = 90.f;

private:
	UPROPERTY(Transient)
	AVRTBeacon* Beacon = nullptr;

	bool bCollected = false;
};
