#pragma once

#include "CoreMinimal.h"

/** Console variables for debug drawing. All default to 0; drawing code must check them first. */
namespace VRTDebug
{
	/** VRT.Debug.Triggers: draw trigger volumes. */
	VRT_API bool ShowTriggers();
	/** VRT.Debug.Holsters: draw holster zones. */
	VRT_API bool ShowHolsters();
	/** VRT.Debug.TwoHand: draw two-hand grip points, aim axis and gun axes. */
	VRT_API bool ShowTwoHand();
	/** VRT.Debug.Aim: draw weapon aim lines. */
	VRT_API bool ShowAimLines();
}
