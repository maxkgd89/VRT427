#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VRFHUD.generated.h"

/**
 * Text HUD for the flat playtest mode (step 11.5). The draw code (HP, keys, exit, level, seed, zoom, free-cam
 * indicator, damage flash) is added in step 11.5.4; the class exists from 11.5.2 so AVRFGameMode can set it.
 */
UCLASS()
class VRT_API AVRFHUD : public AHUD
{
	GENERATED_BODY()
};
