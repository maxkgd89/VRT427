#include "Debug/VRTMazeMapDraw.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Level/VRTMazeData.h"

void VRTMazeMap::DrawMaze(UWorld* World, const FVRTMazeData& Maze, const FVector& Origin, float Scale, float CellSizeCm, float WallHeightCm)
{
	if (!World)
	{
		return;
	}

	const float Cell = CellSizeCm * Scale;
	const float Wall = WallHeightCm * Scale;
	const FColor WallColor = FColor::White;

	auto Corner = [&](float X, float Y, float Z)
	{
		return Origin + FVector(X * Cell, Y * Cell, Z);
	};

	// Two lines per wall (bottom and top edge) keep the line count low on big mazes.
	auto DrawWall = [&](float X0, float Y0, float X1, float Y1)
	{
		DrawDebugLine(World, Corner(X0, Y0, 0.f), Corner(X1, Y1, 0.f), WallColor, true, -1.f, 0, 0.2f);
		DrawDebugLine(World, Corner(X0, Y0, Wall), Corner(X1, Y1, Wall), WallColor, true, -1.f, 0, 0.2f);
	};

	for (int32 Y = 0; Y < Maze.Height; ++Y)
	{
		for (int32 X = 0; X < Maze.Width; ++X)
		{
			const FIntPoint P(X, Y);
			// Each wall is shared, so draw north and west of every cell plus the south / east border.
			if (Maze.HasWall(P, EVRTMazeDir::North)) { DrawWall(X, Y, X + 1, Y); }
			if (Maze.HasWall(P, EVRTMazeDir::West)) { DrawWall(X, Y, X, Y + 1); }
			if (Y == Maze.Height - 1 && Maze.HasWall(P, EVRTMazeDir::South)) { DrawWall(X, Y + 1, X + 1, Y + 1); }
			if (X == Maze.Width - 1 && Maze.HasWall(P, EVRTMazeDir::East)) { DrawWall(X + 1, Y, X + 1, Y + 1); }

			// Cell markers: spawn green, key yellow, dead end orange, room cyan.
			const FVRTMazeCell& C = Maze.CellAt(P);
			const FVector Center = Corner(X + 0.5f, Y + 0.5f, 0.f);
			if (C.HasFlag(EVRTMazeCellFlag::Spawn))
			{
				DrawDebugSphere(World, Center + FVector(0.f, 0.f, Wall * 0.5f), Cell * 0.3f, 8, FColor::Green, true, -1.f, 0, 0.2f);
			}
			else if (C.HasFlag(EVRTMazeCellFlag::Key))
			{
				DrawDebugSphere(World, Center + FVector(0.f, 0.f, Wall * 0.5f), Cell * 0.25f, 8, FColor::Yellow, true, -1.f, 0, 0.2f);
			}
			else if (C.HasFlag(EVRTMazeCellFlag::DeadEnd))
			{
				DrawDebugPoint(World, Center + FVector(0.f, 0.f, 0.5f), Cell * 0.35f, FColor::Orange, true, -1.f, 0);
			}
			else if (C.HasFlag(EVRTMazeCellFlag::Room))
			{
				DrawDebugPoint(World, Center + FVector(0.f, 0.f, 0.5f), Cell * 0.2f, FColor::Cyan, true, -1.f, 0);
			}
		}
	}
}
