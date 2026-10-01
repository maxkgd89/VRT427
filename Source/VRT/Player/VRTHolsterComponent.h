#pragma once

#include "CoreMinimal.h"
#include "Player/VRTGrabPointComponent.h"
#include "VRTHolsterComponent.generated.h"

class AVRTWeaponBase;
class UVRTHandComponent;

/**
 * A body-relative zone (waist, shoulder) the right hand can reach into. It is a grab point that is only
 * open to the right hand. The owning pawn attaches it to the body anchor and calls UpdateForHeadHeight()
 * so the zone scales with the player's height.
 *
 * Offsets are in the body anchor's space (X forward, Y right, Z up), in cm.
 */
UCLASS(ClassGroup = "VRT", meta = (BlueprintSpawnableComponent))
class VRT_API UVRTHolsterComponent : public UVRTGrabPointComponent
{
	GENERATED_BODY()

public:
	UVRTHolsterComponent();

	/** The weapon living in this holster (spawned at BeginPlay), or null. */
	AVRTWeaponBase* GetWeapon() const { return Weapon; }

	/** Re-places the zone for the given head height (cm above the floor). */
	void UpdateForHeadHeight(float HeadHeight);

	/** Weapon spawned into this holster at BeginPlay. Leave empty for an empty zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Holster")
	TSubclassOf<AVRTWeaponBase> WeaponClass;

	/** Zone height = HeightFraction * head height + HeightOffset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Holster")
	float HeightFraction = 1.f;

	/** Added to the zone height, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Holster")
	float HeightOffset = 0.f;

	/** Cm in front of (+) or behind (-) the body anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Holster")
	float ForwardOffset = 0.f;

	/** Cm to the right of (+) or left of (-) the body anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Holster")
	float RightOffset = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Transient)
	AVRTWeaponBase* Weapon = nullptr;

	UFUNCTION()
	void HandleHandEnter(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHandExit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void HandleDrawn(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand);

	UFUNCTION()
	void HandleReleased(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand);
};
