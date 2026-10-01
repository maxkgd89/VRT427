#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRTTriggerBase.generated.h"

class AVRTPawn;
class USphereComponent;

/**
 * Invisible sphere that reacts when the player's pawn walks into it. Subclasses override OnPlayerEntered
 * (keys, level exit). The sphere is drawn with VRT.Debug.Triggers 1, or always with bShowDebugSphere.
 */
UCLASS(Abstract)
class VRT_API AVRTTriggerBase : public AActor
{
	GENERATED_BODY()

public:
	AVRTTriggerBase();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The player's pawn entered / left the trigger sphere. */
	virtual void OnPlayerEntered(AVRTPawn* Pawn) {}
	virtual void OnPlayerExited(AVRTPawn* Pawn) {}

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Trigger")
	USphereComponent* Trigger;

	/** Trigger radius in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Trigger", meta = (ClampMin = "10.0"))
	float TriggerRadius = 90.f;

	/** Draw the trigger sphere even when VRT.Debug.Triggers is off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Trigger")
	bool bShowDebugSphere = false;

	/** Colour of the debug sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Trigger")
	FColor DebugColor = FColor::Cyan;
};
