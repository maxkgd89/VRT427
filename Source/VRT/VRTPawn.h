#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRTPawn.generated.h"

class UCameraComponent;
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
