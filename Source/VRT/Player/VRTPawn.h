#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRTPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UFloatingPawnMovement;
class UMotionControllerComponent;
class UVRTHandComponent;
class UVRTHolsterComponent;
class UStaticMeshComponent;

/** How the player plays: tracking is floor-based when standing, eye-level (recentered) when seated. */
UENUM(BlueprintType)
enum class EVRTPlayMode : uint8
{
	Standing,
	Seated
};

/** VR pawn: HMD camera plus two motion controllers, each with a cube marking the hand. */
UCLASS()
class VRT_API AVRTPawn : public APawn
{
	GENERATED_BODY()

public:
	AVRTPawn();

	/** Short vibration on both controllers (pickups, level feedback). Intensity 0-1, duration in seconds. */
	void PlayHapticPulseBothHands(float Intensity = 0.5f, float DurationSec = 0.15f);

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

	/** Recenters the HMD (RecenterHMD action, left X button): the current head pose becomes the origin and forward. */
	void OnRecenter();

	/**
	 * Places the body anchor under the head, turned by the head's yaw only (pitch and roll ignored), and
	 * re-fits the holsters to the current head height.
	 */
	void UpdateBodyAnchor();

	/** Body yaw (World, degrees). Follows the head slowly, see BodyYawDeadZone and BodyYawFollowSpeed. */
	float BodyYaw = 0.f;

	bool bBodyYawInitialized = false;

	/** Cm from the floor (capsule bottom) up to the tracking origin: 0 standing, SeatedEyeHeight seated. */
	float GetFloorToOriginHeight() const;

	/** Head height (cm above the floor) used for the last holster update. */
	float LastHeadHeight = -1.f;

	void OnFirePressed();
	void OnFireReleased();

	void OnGrabLeftPressed();
	void OnGrabLeftReleased();
	void OnGrabRightPressed();
	void OnGrabRightReleased();

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

	/** Standing uses floor-level tracking. Seated uses eye-level tracking and lifts the origin to SeatedEyeHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	EVRTPlayMode PlayMode = EVRTPlayMode::Seated;

	/** Eye height above the floor in seated mode, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion", meta = (ClampMin = "50.0"))
	float SeatedEyeHeight = 150.f;

	/** Input action that fires the weapon in the right hand. Remap here (e.g. to "FireLeft") without other changes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Weapon")
	FName FireAction = FName("FireRight");

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

	/** Follows the HMD position under the head, yaw only. Holsters hang off this. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	USceneComponent* BodyAnchor;

	/** Right waist zone: holds the pistol (step 5). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UVRTHolsterComponent* WaistHolster;

	/** Right shoulder zone: holds the gun (step 6). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UVRTHolsterComponent* ShoulderHolster;

	/** The body (holsters) only starts turning once the head is turned further than this from it, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Holster", meta = (ClampMin = "0.0"))
	float BodyYawDeadZone = 35.f;

	/** How fast the body turns after the head has left the dead zone, degrees per second. Snap turns rotate it instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Holster", meta = (ClampMin = "1.0"))
	float BodyYawFollowSpeed = 90.f;

	/** Grab sphere on the left controller. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UVRTHandComponent* LeftHand;

	/** Grab sphere on the right controller. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UVRTHandComponent* RightHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UStaticMeshComponent* LeftHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UStaticMeshComponent* RightHandMesh;
};
