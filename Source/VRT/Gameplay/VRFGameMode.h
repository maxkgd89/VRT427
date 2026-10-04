#pragma once

#include "CoreMinimal.h"
#include "Gameplay/VRTGameMode.h"
#include "VRFGameMode.generated.h"

/**
 * Game mode for the flat playtest mode (step 11.5). Identical to AVRTGameMode (same level flow, maze regeneration
 * and death handling) but swaps the pawn, the player controller and the HUD for the flat versions.
 *
 * Selected per map in World Settings > GameMode Override = VRFGameMode.
 */
UCLASS()
class VRT_API AVRFGameMode : public AVRTGameMode
{
	GENERATED_BODY()

public:
	AVRFGameMode();
};
