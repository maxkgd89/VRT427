#pragma once

#include "CoreMinimal.h"

/** How a level's maze grows (plan 10.5 size table). Pure data, so it can be tested without a world. */
struct FVRTLevelSettings
{
	/** Grid edge length in cells (the maze is Size x Size). */
	int32 Size = 8;

	/** Open areas besides the spawn hub. */
	int32 Rooms = 2;

	/** Share of dead ends opened into loops. */
	float Braid = 0.15f;

	/** Growing Tree newest-cell bias. Lower is bushier. */
	float NewestBias = 0.75f;
};

namespace VRTLevelProgression
{
	/** Largest maze the game builds (plan: the upper bound for Quest with AI). */
	constexpr int32 MaxSize = 32;

	/**
	 * Settings for a level (1-based):
	 *   level  1      8 x 8
	 *   levels 2-3   12 x 12
	 *   levels 4-6   16 x 16
	 *   levels 7-9   20 x 20
	 *   levels 10-12 24 x 24
	 *   levels 13-15 28 x 28
	 *   levels 16+   32 x 32
	 * Rooms grow from 2 up to 6, braiding from 15% up to 30%, and the maze gets a little bushier.
	 */
	inline FVRTLevelSettings ForLevel(int32 Level)
	{
		const int32 L = FMath::Max(Level, 1);

		FVRTLevelSettings Settings;
		if (L == 1) { Settings.Size = 8; }
		else if (L <= 3) { Settings.Size = 12; }
		else if (L <= 6) { Settings.Size = 16; }
		else if (L <= 9) { Settings.Size = 20; }
		else if (L <= 12) { Settings.Size = 24; }
		else if (L <= 15) { Settings.Size = 28; }
		else { Settings.Size = MaxSize; }

		Settings.Rooms = FMath::Min(2 + (L - 1) / 3, 6);
		Settings.Braid = FMath::Min(0.15f + 0.015f * (L - 1), 0.30f);
		Settings.NewestBias = FMath::Max(0.75f - 0.02f * (L - 1), 0.55f);
		return Settings;
	}
}
