#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VRFPlayerController.generated.h"

/**
 * Controller for the flat playtest mode (step 11.5). Everything the VR controller has (motion controllers, snap turn,
 * holsters) is absent here. It only shows the mouse cursor, keeps the mouse uncaptured so the cursor can aim, and
 * turns fog off so the top-down view of the maze stays readable.
 */
UCLASS()
class VRT_API AVRFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AVRFPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Value of ShowFlag.Fog before VRF changed it, or -1 when it was never touched. */
	int32 PreviousFogShowFlag = -1;
};
