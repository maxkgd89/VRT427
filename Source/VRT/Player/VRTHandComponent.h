#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "VRTHandComponent.generated.h"

class AVRTWeaponBase;
class UVRTGrabPointComponent;

/**
 * One per hand, attached to a motion controller. A small overlap sphere tracks which grab points are in
 * range. Grip press grabs the closest allowed one, grip release lets go of it.
 */
UCLASS(ClassGroup = "VRT", meta = (BlueprintSpawnableComponent))
class VRT_API UVRTHandComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UVRTHandComponent();

	/** Which hand this component represents. Set by the owning pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Hand")
	EControllerHand Hand = EControllerHand::Left;

	/** Grip pressed: grabs the best candidate in range, if any. */
	void OnGripPressed();

	/** Grip released: drops whatever is held. */
	void OnGripReleased();

	/** Trigger pressed / released: forwarded to the weapon in this hand, if any. */
	void OnTriggerPressed();
	void OnTriggerReleased();

	/** The weapon drawn from a holster and held by this hand, or null. */
	AVRTWeaponBase* GetHeldWeapon() const;

	/** Closest grab point in range that this hand may grab, or null. */
	UVRTGrabPointComponent* GetBestCandidate() const;

	UVRTGrabPointComponent* GetHeldPoint() const { return HeldPoint; }
	bool IsHolding() const { return HeldPoint != nullptr; }

	/** Short vibration on this hand's controller. Intensity 0-1, duration in seconds. */
	void PlayHapticPulse(float Intensity = 0.4f, float DurationSec = 0.08f);

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** Overlap sphere radius in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Hand", meta = (ClampMin = "1.0"))
	float GrabRadius = 8.f;

private:
	void StopHaptics();

	/** Grab points currently overlapping the hand sphere. */
	UPROPERTY(Transient)
	TArray<UVRTGrabPointComponent*> CandidatesInRange;

	UPROPERTY(Transient)
	UVRTGrabPointComponent* HeldPoint = nullptr;

	FTimerHandle HapticTimer;
};
