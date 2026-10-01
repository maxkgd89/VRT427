#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRTPawn.generated.h"

class UCameraComponent;
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
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Right thumbstick X handler; fires one snap turn per stick flick. */
	void OnSnapTurnAxis(float Value);

	/** Rotates the pawn around the HMD position so the player's head doesn't shift. */
	void SnapTurn(float YawDegrees);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Locomotion")
	float SnapTurnAngle = 30.f;

	/** Stick deflection needed to trigger a turn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Locomotion")
	float SnapTurnActivationThreshold = 0.6f;

	/** Stick must return below this before another turn can trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Locomotion")
	float SnapTurnResetThreshold = 0.3f;

	bool bSnapTurnArmed = true;

	void OnMoveForward(float Value);
	void OnMoveRight(float Value);
	void UpdateMoveSpeed();

	/** Left controller's forward/right vectors flattened onto the horizontal plane. */
	FVector GetControllerForwardFlat() const;
	FVector GetControllerRightFlat() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UFloatingPawnMovement* Movement;

	/** Walk speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Locomotion")
	float WalkSpeed = 150.f;

	/** Run speed in cm/s, used while the left thumbstick is pressed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Locomotion")
	float RunSpeed = 350.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	USceneComponent* VROrigin;

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
