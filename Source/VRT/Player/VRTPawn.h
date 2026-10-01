#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRTPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UFloatingPawnMovement;
class UMotionControllerComponent;
class UStaticMeshComponent;

/** VR pawn: HMD camera plus two motion controllers, each with a cube marking the hand. */
UCLASS()
class VRT_API AVRTPawn : public APawn
{
	GENERATED_BODY()

public:
	AVRTPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Handler for the Turn axis (right thumbstick X); fires one snap turn per stick flick. */
	void OnSnapTurnAxis(float Value);

	/** Rotates the pawn around the HMD position so the player's head doesn't shift. */
	void SnapTurn(float YawDegrees);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	float SnapTurnAngle = 30.f;

	/** Stick deflection needed to trigger a turn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	float SnapTurnActivationThreshold = 0.6f;

	/** Stick must return below this before another turn can trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	float SnapTurnResetThreshold = 0.3f;

	bool bSnapTurnArmed = true;

	void OnMoveForward(float Value);
	void OnMoveRight(float Value);
	/** Flips between walk and run (ToggleRun action, left X button). */
	void OnToggleRun();

	/** True while in run mode; toggled by the ToggleRun action. */
	bool bRunning = false;

	/** Applies the radial dead zone to the stored stick input and moves the pawn. */
	void ApplyMoveInput();

	/** Raw left thumbstick values (X = right, Y = forward). */
	FVector2D RawMoveInput = FVector2D::ZeroVector;

	/** Stick deflection below this is ignored (stick drift). Output is rescaled so it still ramps from 0 to 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float MoveDeadZone = 0.2f;

	/** Left controller's forward/right vectors flattened onto the horizontal plane. */
	FVector GetControllerForwardFlat() const;
	FVector GetControllerRightFlat() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UFloatingPawnMovement* Movement;

	/** Walk speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	float WalkSpeed = 150.f;

	/** Run speed in cm/s, used while in run mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	float RunSpeed = 350.f;

	/** Collision volume (root). Follows the HMD's horizontal position every tick. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UCapsuleComponent* Capsule;

	/** Tracking-space origin, parented to the capsule and positioned at floor level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	USceneComponent* VROrigin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Collision")
	float CapsuleRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Collision")
	float CapsuleHalfHeight = 85.f;

	/** Downward acceleration in cm/s^2, so the capsule settles back to the floor after stepping onto something. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Collision")
	float GravityZ = -980.f;

	float VerticalVelocity = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UMotionControllerComponent* LeftController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UStaticMeshComponent* LeftHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UStaticMeshComponent* RightHandMesh;
};
