#pragma once

#include "CoreMinimal.h"

class UWorld;
struct FVRTMazeData;

/** Draws a maze as a small miniature made of debug lines. Used by the in-game dev map and by VRT.Maze.Generate. */
namespace VRTMazeMap
{
	/**
	 * Draws the walls (bottom and top edge of each wall), the spawn cell (green sphere), keys (yellow spheres),
	 * dead ends (orange points) and room cells (cyan points). The lines are persistent: remove them with
	 * FlushPersistentDebugLines.
	 *
	 * @param Origin       World position of the corner of cell (0, 0) (north-west corner), at floor level.
	 * @param Scale        Miniature scale: 0.02 draws a 400 cm cell 8 cm wide.
	 * @param CellSizeCm   Real cell size the maze represents.
	 * @param WallHeightCm Real wall height.
	 */
	VRT_API void DrawMaze(UWorld* World, const FVRTMazeData& Maze, const FVector& Origin, float Scale, float CellSizeCm, float WallHeightCm);
}
