// Console commands to generate a maze and look at it without building any geometry (plan step 10.1).
//
//   VRT.Maze.Generate [Width Height Seed BraidFraction RoomCount NewestBias DrawScale]
//   VRT.Maze.Clear
//   VRT.Maze.SelfTest      (generator checks over many sizes and seeds; no world or headset needed)
//
// The maze is drawn as a miniature with debug lines in front of the player and printed as text to LogVRTMaze.

#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Level/VRTMazeData.h"
#include "Level/VRTMazeGenerator.h"
#include "VRTLog.h"

namespace
{
	/** Real cell size the miniature represents, cm (plan 10.4: 4 m cells). */
	constexpr float CellSizeCm = 400.f;
	constexpr float WallHeightCm = 200.f;

	void DrawMaze(UWorld* World, const FVRTMazeData& Maze, const FVector& Origin, float Scale)
	{
		const float Cell = CellSizeCm * Scale;
		const float Wall = WallHeightCm * Scale;
		const FColor WallColor = FColor::White;

		auto Corner = [&](float X, float Y, float Z)
		{
			return Origin + FVector(X * Cell, Y * Cell, Z);
		};

		auto DrawWall = [&](float X0, float Y0, float X1, float Y1)
		{
			DrawDebugLine(World, Corner(X0, Y0, 0.f), Corner(X1, Y1, 0.f), WallColor, true, -1.f, 0, 0.15f);
			DrawDebugLine(World, Corner(X0, Y0, Wall), Corner(X1, Y1, Wall), WallColor, true, -1.f, 0, 0.15f);
			DrawDebugLine(World, Corner(X0, Y0, 0.f), Corner(X0, Y0, Wall), WallColor, true, -1.f, 0, 0.15f);
			DrawDebugLine(World, Corner(X1, Y1, 0.f), Corner(X1, Y1, Wall), WallColor, true, -1.f, 0, 0.15f);
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

				// Cell markers: spawn green, dead end orange, room cyan.
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

	void GenerateCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		FVRTMazeParams Params;
		float Scale = 0.03f;
		if (Args.IsValidIndex(0)) { Params.Width = FCString::Atoi(*Args[0]); }
		if (Args.IsValidIndex(1)) { Params.Height = FCString::Atoi(*Args[1]); }
		if (Args.IsValidIndex(2)) { Params.Seed = FCString::Atoi(*Args[2]); }
		if (Args.IsValidIndex(3)) { Params.BraidFraction = FCString::Atof(*Args[3]); }
		if (Args.IsValidIndex(4)) { Params.RoomCount = FCString::Atoi(*Args[4]); }
		if (Args.IsValidIndex(5)) { Params.NewestBias = FCString::Atof(*Args[5]); }
		if (Args.IsValidIndex(6)) { Scale = FMath::Max(FCString::Atof(*Args[6]), 0.001f); }

		FVRTMazeStats Stats;
		const FVRTMazeData Maze = FVRTMazeGenerator::Generate(Params, &Stats);

		VRT_LOG(LogVRTMaze, Log, "Generated maze seed=%d grid=%dx%d newestBias=%.2f braid=%.2f rooms=%d(+hub %d) in %.2f ms",
			Maze.Seed, Maze.Width, Maze.Height, Params.NewestBias, Params.BraidFraction, Params.RoomCount, Params.SpawnHubSize, Stats.GenerationMs);
		VRT_LOG(LogVRTMaze, Log, "Rooms carved=%d, dead ends %d -> %d after braiding, reachable %d/%d cells, longest path from spawn=%d cells",
			Stats.RoomsCarved, Stats.DeadEndsBeforeBraid, Stats.DeadEndsAfterBraid, Stats.CellsReachable, Maze.Width * Maze.Height, Maze.MaxDistance);
		for (const FString& Line : Maze.ToAsciiLines())
		{
			VRT_LOG(LogVRTMaze, Log, "%s", *Line);
		}

		// Miniature in front of the player, at chest height; axes match the world (X east, Y south).
		FlushPersistentDebugLines(World);
		FVector Center = FVector::ZeroVector;
		if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			const FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
			Center = Pawn->GetActorLocation() + Forward * (CellSizeCm * Scale * Maze.Height * 0.5f + 80.f) + FVector(0.f, 0.f, 20.f);
		}
		const FVector Origin = Center - FVector(Maze.Width * CellSizeCm * Scale * 0.5f, Maze.Height * CellSizeCm * Scale * 0.5f, 0.f);
		DrawMaze(World, Maze, Origin, Scale);
		VRT_LOG(LogVRTMaze, Log, "Miniature drawn at(World)=%s scale=%.3f (VRT.Maze.Clear removes it)", *Origin.ToCompactString(), Scale);
	}

	void ClearCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (World)
		{
			FlushPersistentDebugLines(World);
		}
	}

	/** Checks one generated maze. Returns an empty string if it is fine, else what is wrong. */
	FString CheckMaze(const FVRTMazeParams& Params, bool bExpectTree)
	{
		const FVRTMazeData Maze = FVRTMazeGenerator::Generate(Params);
		const int32 Cells = Maze.Width * Maze.Height;

		// Every cell reachable from the spawn.
		FVRTMazeData Copy = Maze;
		if (Copy.ComputeDistances(Copy.SpawnCell) != Cells)
		{
			return TEXT("not all cells reachable");
		}

		// Walls agree on both sides, the border is closed, edges are counted.
		int32 OpenEdges = 0;
		for (int32 Y = 0; Y < Maze.Height; ++Y)
		{
			for (int32 X = 0; X < Maze.Width; ++X)
			{
				const FIntPoint P(X, Y);
				for (int32 D = 0; D < VRTMaze::DirCount; ++D)
				{
					const EVRTMazeDir Dir = static_cast<EVRTMazeDir>(D);
					const FIntPoint N = Maze.Neighbor(P, Dir);
					if (!Maze.IsInside(N))
					{
						if (!Maze.HasWall(P, Dir)) { return TEXT("border wall missing"); }
						continue;
					}
					if (Maze.HasWall(P, Dir) != Maze.HasWall(N, VRTMaze::Opposite(Dir))) { return TEXT("wall differs between neighbours"); }
					if (!Maze.HasWall(P, Dir)) { ++OpenEdges; }
				}
			}
		}
		OpenEdges /= 2; // every opening was counted from both sides

		if (bExpectTree && OpenEdges != Cells - 1)
		{
			return FString::Printf(TEXT("expected a perfect maze (%d openings) but found %d"), Cells - 1, OpenEdges);
		}
		if (OpenEdges < Cells - 1)
		{
			return TEXT("fewer openings than a spanning tree needs");
		}

		// Spawn cell is tagged and the same params give the same maze.
		if (!Maze.CellAt(Maze.SpawnCell).HasFlag(EVRTMazeCellFlag::Spawn)) { return TEXT("spawn cell not flagged"); }
		if (FVRTMazeGenerator::Generate(Params).ToAsciiLines() != Maze.ToAsciiLines()) { return TEXT("not deterministic"); }
		return FString();
	}

	void SelfTestCommand(const TArray<FString>& Args)
	{
		const FIntPoint Sizes[] = { FIntPoint(2, 2), FIntPoint(5, 9), FIntPoint(8, 8), FIntPoint(12, 12), FIntPoint(16, 16), FIntPoint(32, 32) };
		int32 Runs = 0, Failures = 0;

		for (const FIntPoint& Size : Sizes)
		{
			for (int32 Seed = 1; Seed <= 20; ++Seed)
			{
				// Three parameter sets: a perfect maze, the defaults, and a loopy one with many rooms.
				for (int32 Variant = 0; Variant < 3; ++Variant)
				{
					FVRTMazeParams Params;
					Params.Width = Size.X;
					Params.Height = Size.Y;
					Params.Seed = Seed;
					bool bExpectTree = false;
					if (Variant == 0) { Params.BraidFraction = 0.f; Params.RoomCount = 0; Params.SpawnHubSize = 1; bExpectTree = true; }
					if (Variant == 2) { Params.BraidFraction = 0.6f; Params.RoomCount = 4; Params.NewestBias = 0.25f; Params.SpawnHubSize = 3; }

					++Runs;
					const FString Problem = CheckMaze(Params, bExpectTree);
					if (!Problem.IsEmpty())
					{
						++Failures;
						VRT_LOG(LogVRTMaze, Error, "SelfTest FAIL grid=%dx%d seed=%d variant=%d: %s", Size.X, Size.Y, Seed, Variant, *Problem);
					}
				}
			}
		}

		if (Failures == 0)
		{
			VRT_LOG(LogVRTMaze, Log, "SelfTest PASS: %d mazes checked (reachability, wall symmetry, border, determinism, spanning tree)", Runs);
		}
		else
		{
			VRT_LOG(LogVRTMaze, Error, "SelfTest FAILED: %d of %d mazes had problems", Failures, Runs);
		}
	}

	FAutoConsoleCommand GMazeSelfTest(
		TEXT("VRT.Maze.SelfTest"),
		TEXT("Generates many mazes and checks them (reachability, wall symmetry, determinism). Result goes to LogVRTMaze."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SelfTestCommand));

	FAutoConsoleCommandWithWorldAndArgs GMazeGenerate(
		TEXT("VRT.Maze.Generate"),
		TEXT("Generates a maze and draws a miniature. Args: [Width Height Seed BraidFraction RoomCount NewestBias DrawScale]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GenerateCommand));

	FAutoConsoleCommandWithWorldAndArgs GMazeClear(
		TEXT("VRT.Maze.Clear"),
		TEXT("Removes the maze miniature drawn by VRT.Maze.Generate."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ClearCommand));
}
