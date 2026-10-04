#pragma once

#include "CoreMinimal.h"
#include "Player/VRTPlayerPawnBase.h"
#include "VRFPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * Flat (top-down) playtest pawn for developing AI and levels on a monitor (step 11.5). Not part of the final game.
 *
 * A cylinder body (the capsule's visual) with a small "nose" box showing which way it faces, seen from a spring-arm
 * camera looking straight down. WASD moves relative to the camera yaw (W = up on screen), the wheel zooms.
 * Movement is horizontal only and has no gravity; walls block the capsule, exactly like the VR pawn.
 */
UCLASS()
class VRT_API AVRFPawn : public AVRTPlayerPawnBase
{
	GENERATED_BODY()

public:
	AVRFPawn();

	/** Restores health (base) and stops any leftover movement. */
	virtual void NotifyTeleported() override;

	/** Reference point enemies trace their sight to: the floor plus EyeHeight, never the camera (which is far above). */
	virtual FVector GetPawnViewLocation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** WASD axis handlers. They add movement relative to the on-screen up/right directions. */
	void OnMoveForward(float Value);
	void OnMoveRight(float Value);

	/** VRF_Run is held (Shift), not toggled: pressed → run speed, released → walk speed. */
	void OnRunPressed();
	void OnRunReleased();

	/** Scroll wheel: one notch multiplies the target arm length by ZoomStep. */
	void OnZoom(float Value);

	/** Eases the spring arm toward the target length. */
	void UpdateZoom(float DeltaSeconds);

	/** On-screen up (W) as a floor direction. The camera looks straight down, so its forward axis is vertical. */
	FVector GetViewForwardFlat() const;

	/** On-screen right (D) as a floor direction. */
	FVector GetViewRightFlat() const;

	/** Absolute-rotation spring arm: the view stays fixed whatever way the pawn faces. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRF|Camera")
	USpringArmComponent* CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRF|Camera")
	UCameraComponent* Camera;

	/** Cylinder showing the player's body; collision stays on the capsule. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRF|Body")
	UStaticMeshComponent* BodyMesh;

	/** Small box at the front edge: shows the pawn's facing from above. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRF|Body")
	UStaticMeshComponent* NoseMesh;

	/** Camera pitch in degrees. -90 is straight down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera")
	float CameraPitch = -90.f;

	/** Camera yaw in degrees. Fixed: the view never follows the pawn, only WASD movement uses its direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera")
	float CameraYaw = -90.f;

	/** Arm length a level starts with, cm (the camera's height above the pawn). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera", meta = (ClampMin = "10.0"))
	float StartArmLength = 2000.f;

	/** Closest the camera may come to the pawn, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera", meta = (ClampMin = "10.0"))
	float MinArm = 400.f;

	/** Furthest the camera may go, cm. 12000 cm shows a whole 32x32 maze. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera", meta = (ClampMin = "10.0"))
	float MaxArm = 12000.f;

	/** Arm length factor per scroll notch (higher zooms faster). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera", meta = (ClampMin = "1.01"))
	float ZoomStep = 1.15f;

	/** How fast the arm reaches the target length; higher is snappier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 8.f;

	/** Eye height above the floor for GetPawnViewLocation(), cm (same value as the VR pawn's SeatedEyeHeight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRF|Camera", meta = (ClampMin = "50.0"))
	float EyeHeight = 150.f;

	/** Arm length the zoom is easing toward, cm. */
	float DesiredArmLength = 2000.f;

	/** True while VRF_Run is held. */
	bool bRunning = false;
};
