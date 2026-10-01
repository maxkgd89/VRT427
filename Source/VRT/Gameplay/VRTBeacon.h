#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRTBeacon.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Tall blue unlit column that marks a key from far away, above the labyrinth walls. No collision, no shadow.
 * The actor origin is the base of the column (on the floor).
 */
UCLASS()
class VRT_API AVRTBeacon : public AActor
{
	GENERATED_BODY()

public:
	AVRTBeacon();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Narrows the column to nothing over DurationSec, then hides the beacon. */
	void FadeOut(float DurationSec = 1.f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Applies Height and Radius to the mesh. */
	void ApplyShape(float RadiusScale = 1.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Beacon")
	UStaticMeshComponent* Mesh;

	/** Column height in cm (10 to 20 m). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Beacon", meta = (ClampMin = "1000.0", ClampMax = "2000.0"))
	float Height = 1500.f;

	/** Column radius in cm (25 = 0.5 m wide). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Beacon", meta = (ClampMin = "5.0"))
	float Radius = 25.f;

	/** Emissive colour of the column (linear, values above 1 glow). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Beacon")
	FLinearColor Color = FLinearColor(0.05f, 0.3f, 4.f, 1.f);

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* Material = nullptr;

	bool bFadingOut = false;
	float FadeDuration = 1.f;
	float FadeElapsed = 0.f;
};
