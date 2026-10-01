#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VRTGameMode.generated.h"

/** Game mode that spawns AVRTPawn for the player. */
UCLASS()
class VRT_API AVRTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVRTGameMode();
};
