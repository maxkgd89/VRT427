// Console commands to generate a maze and look at it without building any geometry (plan step 10.1).
//
//   VRT.Maze.Generate [Width Height Seed BraidFraction RoomCount NewestBias DrawScale]
//   VRT.Maze.Clear
//   VRT.Maze.Build [Width Height Seed BraidFraction RoomCount NewestBias]   (rebuilds the AVRTMazeBuilder in the level and moves you to its spawn)
//   VRT.Maze.Level N       (fades out and goes to level N of the maze progression: size, seed and rooms follow the level)
//   VRT.Nav.Status         (is the navigation mesh building? how big is its area?)
//   VRT.Nav.Test          (check again that the spawn reaches every key on the navigation mesh)
//   VRT.Maze.SelfTest      (generator checks over many sizes and seeds; no world or headset needed)
//
// The maze is drawn as a miniature with debug lines in front of the player and printed as text to LogVRTMaze.

#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Debug/VRTMazeMapDraw.h"
#include "Gameplay/VRTGameMode.h"
#include "Level/VRTLevelProgression.h"
#include "Level/VRTMazeBuilder.h"
#include "Level/VRTMazeData.h"
#include "Level/VRTMazeGenerator.h"
#include "VRTLog.h"

namespace
{
	/** Real cell size the miniature represents, cm (plan 10.4: 4 m cells). */
	constexpr float MazeCellSizeCm = 400.f;
	constexpr float MazeWallHeightCm = 200.f;

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
		VRT_LOG(LogVRTMaze, Log, "Rooms carved=%d, keys placed=%d, dead ends %d -> %d after braiding, reachable %d/%d cells, longest path from spawn=%d cells",
			Stats.RoomsCarved, Stats.KeysPlaced, Stats.DeadEndsBeforeBraid, Stats.DeadEndsAfterBraid, Stats.CellsReachable, Maze.Width * Maze.Height, Maze.MaxDistance);
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
			Center = Pawn->GetActorLocation() + Forward * (MazeCellSizeCm * Scale * Maze.Height * 0.5f + 80.f) + FVector(0.f, 0.f, 20.f);
		}
		const FVector Origin = Center - FVector(Maze.Width * MazeCellSizeCm * Scale * 0.5f, Maze.Height * MazeCellSizeCm * Scale * 0.5f, 0.f);
		VRTMazeMap::DrawMaze(World, Maze, Origin, Scale, MazeCellSizeCm, MazeWallHeightCm);
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

		// Keys: the right number, spread out, away from the spawn, all reachable.
		TArray<FIntPoint> Keys;
		for (int32 Y = 0; Y < Maze.Height; ++Y)
		{
			for (int32 X = 0; X < Maze.Width; ++X)
			{
				if (Maze.CellAt(FIntPoint(X, Y)).HasFlag(EVRTMazeCellFlag::Key)) { Keys.Add(FIntPoint(X, Y)); }
			}
		}
		const int32 ExpectedKeys = FMath::Clamp(Params.KeyCount, 0, 4);
		if (Cells >= 36 && Keys.Num() != ExpectedKeys) { return FString::Printf(TEXT("expected %d keys, found %d"), ExpectedKeys, Keys.Num()); }
		if (Keys.Num() > ExpectedKeys) { return TEXT("too many keys"); }
		for (const FIntPoint& Key : Keys)
		{
			if (Key == Maze.SpawnCell) { return TEXT("key in the spawn cell"); }
			if (Maze.CellAt(Key).Distance <= 0) { return TEXT("key not reachable from the spawn"); }
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

		// Every level of the progression (1-20) must produce a valid maze, within the size cap.
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			const FVRTLevelSettings Settings = VRTLevelProgression::ForLevel(Level);
			FVRTMazeParams Params;
			Params.Width = Settings.Size;
			Params.Height = Settings.Size;
			Params.NewestBias = Settings.NewestBias;
			Params.BraidFraction = Settings.Braid;
			Params.RoomCount = Settings.Rooms;
			Params.Seed = 1 + Level - 1;

			++Runs;
			FString Problem = Settings.Size > VRTLevelProgression::MaxSize ? FString(TEXT("level size above the cap")) : CheckMaze(Params, false);
			if (!Problem.IsEmpty())
			{
				++Failures;
				VRT_LOG(LogVRTMaze, Error, "SelfTest FAIL progression level %d (%dx%d): %s", Level, Settings.Size, Settings.Size, *Problem);
			}
		}

		if (Failures == 0)
		{
			VRT_LOG(LogVRTMaze, Log, "SelfTest PASS: %d mazes checked (reachability, wall symmetry, border, determinism, spanning tree, key placement, level progression 1-20)", Runs);
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

	void BuildCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		AVRTMazeBuilder* Builder = nullptr;
		for (TActorIterator<AVRTMazeBuilder> It(World); It; ++It)
		{
			Builder = *It;
			break;
		}
		if (!Builder)
		{
			VRT_LOG(LogVRTMaze, Warning, "No AVRTMazeBuilder in this level (open the maze map, see Tools/Editor/create_maze_level.py)");
			return;
		}

		if (Args.IsValidIndex(0)) { Builder->MazeWidth = FMath::Clamp(FCString::Atoi(*Args[0]), 2, 64); }
		if (Args.IsValidIndex(1)) { Builder->MazeHeight = FMath::Clamp(FCString::Atoi(*Args[1]), 2, 64); }
		if (Args.IsValidIndex(2)) { Builder->Seed = FCString::Atoi(*Args[2]); }
		if (Args.IsValidIndex(3)) { Builder->BraidFraction = FMath::Clamp(FCString::Atof(*Args[3]), 0.f, 1.f); }
		if (Args.IsValidIndex(4)) { Builder->RoomCount = FMath::Max(0, FCString::Atoi(*Args[4])); }
		if (Args.IsValidIndex(5)) { Builder->NewestBias = FMath::Clamp(FCString::Atof(*Args[5]), 0.f, 1.f); }

		// Explicit parameters replace the level progression until VRT.Maze.Level turns it back on.
		Builder->bUseLevelProgression = false;
		const AVRTGameMode* GameMode = World->GetAuthGameMode<AVRTGameMode>();
		Builder->GenerateAndBuild(GameMode ? GameMode->GetLevelIndex() : 1);
		Builder->PlacePawnAtSpawn(UGameplayStatics::GetPlayerPawn(World, 0));
	}

	void LevelCommand(const TArray<FString>& Args, UWorld* World)
	{
		AVRTGameMode* GameMode = World ? World->GetAuthGameMode<AVRTGameMode>() : nullptr;
		if (!GameMode || Args.Num() < 1)
		{
			VRT_LOG(LogVRTMaze, Warning, "Usage: VRT.Maze.Level <level number> (needs the game mode of a running game)");
			return;
		}

		for (TActorIterator<AVRTMazeBuilder> It(World); It; ++It)
		{
			It->bUseLevelProgression = true;
		}
		GameMode->GoToLevel(FMath::Max(1, FCString::Atoi(*Args[0])));
	}

	FAutoConsoleCommandWithWorldAndArgs GMazeLevel(
		TEXT("VRT.Maze.Level"),
		TEXT("Goes to level N of the maze progression (fade, new maze with that level's size and seed). Args: <level>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LevelCommand));

	FAutoConsoleCommandWithWorldAndArgs GMazeBuild(
		TEXT("VRT.Maze.Build"),
		TEXT("Rebuilds the maze geometry of the AVRTMazeBuilder in the level. Args: [Width Height Seed BraidFraction RoomCount NewestBias]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BuildCommand));

	AVRTMazeBuilder* FindBuilder(UWorld* World)
	{
		for (TActorIterator<AVRTMazeBuilder> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GNavStatus(
		TEXT("VRT.Nav.Status"),
		TEXT("Logs the state of the navigation mesh (LogVRTNav)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (AVRTMazeBuilder* Builder = World ? FindBuilder(World) : nullptr)
			{
				Builder->LogNavigationStatus();
			}
			else
			{
				VRT_LOG(LogVRTNav, Warning, "No AVRTMazeBuilder in this level");
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GNavTest(
		TEXT("VRT.Nav.Test"),
		TEXT("Checks that the navigation mesh connects the spawn to every key (LogVRTNav)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (AVRTMazeBuilder* Builder = World ? FindBuilder(World) : nullptr)
			{
				Builder->ValidateNavigation();
			}
			else
			{
				VRT_LOG(LogVRTNav, Warning, "No AVRTMazeBuilder in this level");
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GMazeClear(
		TEXT("VRT.Maze.Clear"),
		TEXT("Removes the maze miniature drawn by VRT.Maze.Generate."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ClearCommand));
}
