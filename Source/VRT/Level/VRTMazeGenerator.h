#pragma once

#include "CoreMinimal.h"
#include "Level/VRTMazeData.h"

/** Everything that decides what a generated maze looks like. Same params and seed give the same maze. */
struct FVRTMazeParams
{
	int32 Width = 8;
	int32 Height = 8;

	/** Drives every random choice (FRandomStream). Level N can use Seed + N. */
	int32 Seed = 1;

	/**
	 * Growing Tree: chance to continue from the newest cell (like a depth-first search: long winding
	 * corridors). Otherwise a random active cell is used (like Prim's: bushy, many short branches).
	 */
	float NewestBias = 0.75f;

	/** Fraction (0-1) of dead ends removed afterwards by knocking out a wall. This creates loops. */
	float BraidFraction = 0.2f;

	/** Open areas carved before the maze, besides the spawn hub. */
	int32 RoomCount = 2;

	/** Room size limits in cells (inclusive). */
	FIntPoint RoomMinSize = FIntPoint(2, 2);
	FIntPoint RoomMaxSize = FIntPoint(4, 4);

	/** The spawn hub is a SpawnHubSize x SpawnHubSize open area in the bottom-left corner. 1 means no hub. */
	int32 SpawnHubSize = 2;
};

/** What the generator did, for logs and tests. */
struct FVRTMazeStats
{
	int32 DeadEndsBeforeBraid = 0;
	int32 DeadEndsAfterBraid = 0;
	int32 RoomsCarved = 0;
	int32 CellsReachable = 0;
	double GenerationMs = 0.0;
};

/**
 * Growing Tree maze generator with rooms, a spawn hub and braiding.
 *
 * Order: carve the spawn hub and rooms (pre-connected areas), run Growing Tree from the spawn cell (entering
 * a room connects all of it at once), then braid dead ends and compute walking distances from the spawn.
 */
class VRT_API FVRTMazeGenerator
{
public:
	/** Builds a maze. OutStats is optional. Every cell is reachable from the spawn cell by construction. */
	static FVRTMazeData Generate(const FVRTMazeParams& Params, FVRTMazeStats* OutStats = nullptr);

private:
	/** Opens all walls inside the rectangle, tags its cells as a room and appends them to Rooms. */
	static void CarveRoom(FVRTMazeData& Maze, const FIntPoint& Min, const FIntPoint& Size, int32 RoomIndex, TArray<TArray<FIntPoint>>& Rooms);
};
