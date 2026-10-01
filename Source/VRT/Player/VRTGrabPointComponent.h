#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "VRTGrabPointComponent.generated.h"

class UVRTHandComponent;

/** Which hand may grab a grab point. */
UENUM(BlueprintType)
enum class EVRTHandFilter : uint8
{
	Any,
	LeftOnly,
	RightOnly
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVRTGrabPointEvent, UVRTGrabPointComponent*, GrabPoint, UVRTHandComponent*, Hand);

/**
 * A spot a hand can grab: a holster zone, a barrel grip point or a debug target.
 * The hand finds it through sphere overlaps (profile VRTGrabPoint) and picks the closest one on grip press.
 */
UCLASS(ClassGroup = "VRT", meta = (BlueprintSpawnableComponent))
class VRT_API UVRTGrabPointComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UVRTGrabPointComponent();

	/** True if this point is enabled, free and the hand is allowed to grab it. */
	bool CanBeGrabbedBy(const UVRTHandComponent* Hand) const;

	/** Called by the hand. Not meant to be called from gameplay code. */
	void NotifyGrabbed(UVRTHandComponent* Hand);
	void NotifyReleased(UVRTHandComponent* Hand);

	/** Sets the grab zone radius in cm (also usable before registration). */
	void SetGrabRadius(float NewRadius);

	UVRTHandComponent* GetHeldBy() const { return HeldBy; }
	bool IsHeld() const { return HeldBy != nullptr; }

	/** Fired when a hand grabs / lets go of this point. */
	UPROPERTY(BlueprintAssignable, Category = "VRT|Grab")
	FVRTGrabPointEvent OnPointGrabbed;

	UPROPERTY(BlueprintAssignable, Category = "VRT|Grab")
	FVRTGrabPointEvent OnPointReleased;

	/** Identifier shown in logs, e.g. "HolsterWaist". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Grab")
	FName PointId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Grab")
	EVRTHandFilter AllowedHand = EVRTHandFilter::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRT|Grab")
	bool bGrabEnabled = true;

protected:
	virtual void OnRegister() override;

	/** Grab zone radius in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Grab", meta = (ClampMin = "1.0"))
	float GrabRadius = 15.f;

private:
	UPROPERTY(Transient)
	UVRTHandComponent* HeldBy = nullptr;
};
