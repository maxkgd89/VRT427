#pragma once

#include "CoreMinimal.h"

/**
 * Directions on the maze grid. Grid space: X grows to the East, Y grows to the South (row 0 is the top).
 * World space uses the same axes: cell (X, Y) is at (X * CellSize, Y * CellSize).
 */
enum class EVRTMazeDir : uint8
{
	North, // (0, -1)
	East,  // (+1, 0)
	South, // (0, +1)
	West   // (-1, 0)
};

namespace VRTMaze
{
	constexpr int32 DirCount = 4;

	/** Grid offset of a direction. */
	VRT_API FIntPoint DirOffset(EVRTMazeDir Dir);

	/** Wall bit of a direction inside FVRTMazeCell::Walls. */
	VRT_API uint8 DirBit(EVRTMazeDir Dir);

	VRT_API EVRTMazeDir Opposite(EVRTMazeDir Dir);
	VRT_API const TCHAR* DirName(EVRTMazeDir Dir);
}

/** Bits of FVRTMazeCell::Flags. */
namespace EVRTMazeCellFlag
{
	enum Type : uint8
	{
		None = 0,
		Spawn = 1 << 0,
		Key = 1 << 1,
		DeadEnd = 1 << 2,
		Room = 1 << 3
	};
}

/** One grid cell. Walls has one bit per direction (see VRTMaze::DirBit); a set bit means the wall exists. */
struct FVRTMazeCell
{
	uint8 Walls = 0x0F;
	uint8 Flags = EVRTMazeCellFlag::None;

	/** Room index for room cells (0 is the spawn hub), INDEX_NONE for corridor cells. Later: key regions. */
	int32 Region = INDEX_NONE;

	/** Walking distance in cells from the spawn cell (INDEX_NONE until ComputeDistances is called). */
	int32 Distance = INDEX_NONE;

	bool HasFlag(EVRTMazeCellFlag::Type Flag) const { return (Flags & Flag) != 0; }
};

/**
 * The maze as plain data: no actors, no UObjects. It can be generated, tested, drawn and reused (AI flow
 * distance) without a world. Deterministic: the same FVRTMazeParams (including the seed) always produce the
 * same maze.
 */
struct VRT_API FVRTMazeData
{
	int32 Width = 0;
	int32 Height = 0;
	int32 Seed = 0;
	FIntPoint SpawnCell = FIntPoint::ZeroValue;

	/** Largest value in Cells[].Distance after ComputeDistances. */
	int32 MaxDistance = 0;

	/** Row-major: Cells[Y * Width + X]. */
	TArray<FVRTMazeCell> Cells;

	/** Resets to a Width x Height grid with all walls present. */
	void Init(int32 InWidth, int32 InHeight, int32 InSeed);

	bool IsInside(const FIntPoint& P) const { return P.X >= 0 && P.Y >= 0 && P.X < Width && P.Y < Height; }
	int32 Index(const FIntPoint& P) const { return P.Y * Width + P.X; }

	FVRTMazeCell& CellAt(const FIntPoint& P) { return Cells[Index(P)]; }
	const FVRTMazeCell& CellAt(const FIntPoint& P) const { return Cells[Index(P)]; }

	FIntPoint Neighbor(const FIntPoint& P, EVRTMazeDir Dir) const { return P + VRTMaze::DirOffset(Dir); }

	bool HasWall(const FIntPoint& P, EVRTMazeDir Dir) const { return (CellAt(P).Walls & VRTMaze::DirBit(Dir)) != 0; }

	/** Removes the wall between P and its neighbour in Dir (both sides). The neighbour must exist. */
	void RemoveWall(const FIntPoint& P, EVRTMazeDir Dir);

	/** Number of open sides (0-4). Border walls count as closed. */
	int32 OpenCount(const FIntPoint& P) const;

	/** Breadth-first walking distance from Start into Cells[].Distance and MaxDistance. Returns cells reached. */
	int32 ComputeDistances(const FIntPoint& Start);

	/** Cells whose Flags contain Flag. */
	int32 CountFlag(EVRTMazeCellFlag::Type Flag) const;

	/** Recomputes the DeadEnd flag: corridor cells with exactly one open side (room cells never count). */
	void RefreshDeadEnds();

	/** Cell centre in world space, cm. Z is 0 (the floor). */
	FVector CellCenter(const FIntPoint& P, float CellSize) const;

	/** One text line per row, 3 characters per cell, for logs: S spawn, D dead end, r room, K key. */
	TArray<FString> ToAsciiLines() const;
};
