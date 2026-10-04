#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRTPlayerPawnBase.generated.h"

class UCapsuleComponent;
class UFloatingPawnMovement;
class UVRTHealthComponent;

/**
 * What the VR pawn (AVRTPawn) and the flat test pawn (AVRFPawn) have in common: the capsule, the movement component,
 * health, the walk and run speeds and the death handling. Gameplay code outside Player/ talks to this class only.
 */
UCLASS(Abstract)
class VRT_API AVRTPlayerPawnBase : public APawn
{
	GENERATED_BODY()

public:
	AVRTPlayerPawnBase();

	/** Short vibration on both controllers. Does nothing in the base (the flat pawn has no controllers). Intensity 0-1. */
	virtual void PlayHapticPulseBothHands(float Intensity = 0.5f, float DurationSec = 0.15f) {}

	/** Call after the pawn was moved to a new place (new level or restart): back to full health and alive. */
	virtual void NotifyTeleported();

	/** Horizontal direction the player faces (unit vector, World). The VR pawn uses the head, the flat pawn its aim. */
	virtual FVector GetFacingDirectionFlat() const;

	/** Cm from the floor up to the capsule centre. */
	float GetCapsuleHalfHeight() const { return CapsuleHalfHeight; }

	UVRTHealthComponent* GetHealthComponent() const { return Health; }

	/** True from death until the level is rebuilt. */
	bool IsDead() const { return bDead; }

protected:
	virtual void BeginPlay() override;

	/** Called after a hit that did not kill (feedback: haptics, screen flash). */
	virtual void OnDamagedFeedback(float Damage, float NewHealth, AActor* DamageCauser) {}

	/** Called once when health reaches 0, before the level restart starts. */
	virtual void OnDied(AActor* DamageCauser) {}

	UFUNCTION()
	void HandleDamaged(float Damage, float NewHealth, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AActor* DamageCauser);

	/** True from death until the level is rebuilt: no movement, no shooting, no grabbing. */
	bool bDead = false;

	/** Collision volume (root). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UCapsuleComponent* Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UFloatingPawnMovement* Movement;

	/** Player health. Damage arrives through the UE damage flow (bullets, later enemy hits). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UVRTHealthComponent* Health;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Collision")
	float CapsuleRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Collision")
	float CapsuleHalfHeight = 85.f;

	/** Walk speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	float WalkSpeed = 150.f;

	/** Run speed in cm/s, used while in run mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Locomotion")
	float RunSpeed = 350.f;
};
