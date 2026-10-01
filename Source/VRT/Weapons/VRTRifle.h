#pragma once

#include "CoreMinimal.h"
#include "Weapons/VRTWeaponBase.h"
#include "VRTRifle.generated.h"

class UVRTGrabPointComponent;
class UVRTHandComponent;

/** How many hands hold the gun. */
UENUM(BlueprintType)
enum class EVRTGripState : uint8
{
	None,
	OneHand,
	TwoHand
};

/**
 * The gun: two-handed, fast, full-auto. The right hand draws it from the shoulder holster and holds it by
 * the grip. The left hand can then grab the barrel grip point; while it does, the gun aims along the line
 * from the right hand to the left hand. Box about 4 x 4 x 40 cm.
 *
 * Aim (TwoHand): forward = (left - right) normalized; up = right controller's up projected onto the plane
 * perpendicular to forward; position stays at the right grip.
 */
UCLASS()
class VRT_API AVRTRifle : public AVRTWeaponBase
{
	GENERATED_BODY()

public:
	AVRTRifle();

	virtual void AttachToHand(UVRTHandComponent* Hand) override;
	virtual void ReturnToHolster() override;

	EVRTGripState GetGripState() const { return GripState; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleBarrelGrabbed(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand);

	UFUNCTION()
	void HandleBarrelReleased(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand);

	/** Left-hand grab zone on the front half of the gun. Only enabled while the right hand holds the gun. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|TwoHand")
	UVRTGrabPointComponent* BarrelGripPoint;

	/** Below this hand-to-hand distance the aim line is unreliable and the gun follows the right controller, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|TwoHand", meta = (ClampMin = "1.0"))
	float MinTwoHandDistance = 10.f;

	/** Warn if the left hand is further than this from the barrel grip point, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|TwoHand", meta = (ClampMin = "1.0"))
	float StretchWarnDistance = 20.f;

	/** Warn if the gun rotates more than this in one frame, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|TwoHand", meta = (ClampMin = "1.0"))
	float JumpWarnAngle = 45.f;

private:
	void SetGripState(EVRTGripState NewState, const TCHAR* Reason);

	/** Re-attaches to the right hand at the one-handed grip pose. */
	void AttachToRightHand();

	/** Drives the gun transform from both hands. Called every tick in TwoHand. */
	void UpdateTwoHandAim(float DeltaSeconds);

	/** Converts a World position to the pawn's local space for logging. */
	FVector ToPawnLocal(const FVector& WorldPosition) const;

	UPROPERTY(Transient)
	EVRTGripState GripState = EVRTGripState::None;

	/** Previous frame's resolved up vector and rotation (World), used when the current frame is degenerate. */
	FVector LastUp = FVector::UpVector;
	FQuat LastRotation = FQuat::Identity;
	bool bHasLastRotation = false;

	// One warning per occurrence, re-armed when the condition clears.
	bool bWarnedClose = false;
	bool bWarnedUp = false;
	bool bWarnedJump = false;
	bool bWarnedStretch = false;
};
