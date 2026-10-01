#pragma once

#include "CoreMinimal.h"
#include "Weapons/VRTWeaponBase.h"
#include "VRTPistol.generated.h"

/** One-handed, slow, semi-automatic: one shot per trigger press. Box about 4 x 3 x 20 cm. */
UCLASS()
class VRT_API AVRTPistol : public AVRTWeaponBase
{
	GENERATED_BODY()

public:
	AVRTPistol();
};
