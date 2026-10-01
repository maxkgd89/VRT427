#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRTDebugGrabTarget.generated.h"

class UStaticMeshComponent;
class UVRTGrabPointComponent;
class UVRTHandComponent;

/**
 * Test target for step 3: a small sphere with a grab point. The sphere grows while it is held.
 * Place it in a level and set AllowedHand on the GrabPoint to test left-only / right-only filtering.
 */
UCLASS()
class VRT_API AVRTDebugGrabTarget : public AActor
{
	GENERATED_BODY()

public:
	AVRTDebugGrabTarget();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleGrabbed(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand);

	UFUNCTION()
	void HandleReleased(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Debug")
	UVRTGrabPointComponent* GrabPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Debug")
	UStaticMeshComponent* Mesh;
};
