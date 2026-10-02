#include "Level/VRTMazeGenerator.h"
#include "HAL/PlatformTime.h"
#include "Math/RandomStream.h"
#include "VRTLog.h"

void FVRTMazeGenerator::CarveRoom(FVRTMazeData& Maze, const FIntPoint& Min, const FIntPoint& Size, int32 RoomIndex, TArray<TArray<FIntPoint>>& Rooms)
{
	TArray<FIntPoint>& Cells = Rooms.AddDefaulted_GetRef();
	for (int32 Y = Min.Y; Y < Min.Y + Size.Y; ++Y)
	{
		for (int32 X = Min.X; X < Min.X + Size.X; ++X)
		{
			const FIntPoint P(X, Y);
			FVRTMazeCell& Cell = Maze.CellAt(P);
			Cell.Flags |= EVRTMazeCellFlag::Room;
			Cell.Region = RoomIndex;
			Cells.Add(P);

			// Open the east and south sides that stay inside the rectangle.
			if (X + 1 < Min.X + Size.X)
			{
				Maze.RemoveWall(P, EVRTMazeDir::East);
			}
			if (Y + 1 < Min.Y + Size.Y)
			{
				Maze.RemoveWall(P, EVRTMazeDir::South);
			}
		}
	}
}

FVRTMazeData FVRTMazeGenerator::Generate(const FVRTMazeParams& Params, FVRTMazeStats* OutStats)
{
	const double StartTime = FPlatformTime::Seconds();

	const int32 Width = FMath::Max(Params.Width, 2);
	const int32 Height = FMath::Max(Params.Height, 2);

	FVRTMazeData Maze;
	Maze.Init(Width, Height, Params.Seed);
	FRandomStream Random(Params.Seed);

	// --- Rooms: the spawn hub first, then random rooms that keep a one-cell gap to each other. ---
	TArray<TArray<FIntPoint>> Rooms;
	const int32 HubSize = FMath::Clamp(Params.SpawnHubSize, 1, FMath::Min(Width, Height));
	Maze.SpawnCell = FIntPoint(0, Height - 1);
	if (HubSize >= 2)
	{
		CarveRoom(Maze, FIntPoint(0, Height - HubSize), FIntPoint(HubSize, HubSize), 0, Rooms);
	}
	Maze.CellAt(Maze.SpawnCell).Flags |= EVRTMazeCellFlag::Spawn;

	auto Overlaps = [&](const FIntPoint& Min, const FIntPoint& Size)
	{
		// Rooms must not touch: expand each existing room by one cell and test for intersection.
		for (const TArray<FIntPoint>& Room : Rooms)
		{
			FIntPoint RoomMin(MAX_int32, MAX_int32), RoomMax(MIN_int32, MIN_int32);
			for (const FIntPoint& C : Room)
			{
				RoomMin.X = FMath::Min(RoomMin.X, C.X); RoomMin.Y = FMath::Min(RoomMin.Y, C.Y);
				RoomMax.X = FMath::Max(RoomMax.X, C.X); RoomMax.Y = FMath::Max(RoomMax.Y, C.Y);
			}
			const bool bSeparate = Min.X + Size.X - 1 < RoomMin.X - 1 || Min.X > RoomMax.X + 1 ||
				Min.Y + Size.Y - 1 < RoomMin.Y - 1 || Min.Y > RoomMax.Y + 1;
			if (!bSeparate)
			{
				return true;
			}
		}
		return false;
	};

	// Rooms stay well below the maze size so corridors remain between them.
	const int32 MaxRoomW = FMath::Clamp(Params.RoomMaxSize.X, 2, FMath::Max(2, Width / 2));
	const int32 MaxRoomH = FMath::Clamp(Params.RoomMaxSize.Y, 2, FMath::Max(2, Height / 2));
	const int32 MinRoomW = FMath::Clamp(Params.RoomMinSize.X, 2, MaxRoomW);
	const int32 MinRoomH = FMath::Clamp(Params.RoomMinSize.Y, 2, MaxRoomH);

	int32 RoomsCarved = 0;
	for (int32 RoomIndex = 0; RoomIndex < Params.RoomCount; ++RoomIndex)
	{
		for (int32 Attempt = 0; Attempt < 50; ++Attempt)
		{
			const FIntPoint Size(Random.RandRange(MinRoomW, MaxRoomW), Random.RandRange(MinRoomH, MaxRoomH));
			const FIntPoint Min(Random.RandRange(0, Width - Size.X), Random.RandRange(0, Height - Size.Y));
			if (!Overlaps(Min, Size))
			{
				CarveRoom(Maze, Min, Size, Rooms.Num(), Rooms);
				++RoomsCarved;
				break;
			}
		}
	}

	// --- Growing Tree from the spawn cell. Entering a room connects the whole room at once. ---
	TArray<bool> Visited;
	Visited.Init(false, Width * Height);
	TArray<FIntPoint> Active;

	auto Visit = [&](const FIntPoint& P)
	{
		if (Visited[Maze.Index(P)])
		{
			return;
		}
		Visited[Maze.Index(P)] = true;
		Active.Add(P);
		const int32 Region = Maze.CellAt(P).Region;
		if (Region != INDEX_NONE)
		{
			for (const FIntPoint& RoomCell : Rooms[Region])
			{
				if (!Visited[Maze.Index(RoomCell)])
				{
					Visited[Maze.Index(RoomCell)] = true;
					Active.Add(RoomCell);
				}
			}
		}
	};

	Visit(Maze.SpawnCell);
	while (Active.Num() > 0)
	{
		const int32 ActiveIndex = Random.FRand() < Params.NewestBias ? Active.Num() - 1 : Random.RandRange(0, Active.Num() - 1);
		const FIntPoint Current = Active[ActiveIndex];

		TArray<EVRTMazeDir, TInlineAllocator<4>> Options;
		for (int32 D = 0; D < VRTMaze::DirCount; ++D)
		{
			const EVRTMazeDir Dir = static_cast<EVRTMazeDir>(D);
			const FIntPoint N = Maze.Neighbor(Current, Dir);
			if (Maze.IsInside(N) && !Visited[Maze.Index(N)])
			{
				Options.Add(Dir);
			}
		}

		if (Options.Num() == 0)
		{
			Active.RemoveAt(ActiveIndex); // keeps the order, so "newest" stays the last element
			continue;
		}

		const EVRTMazeDir Chosen = Options[Random.RandRange(0, Options.Num() - 1)];
		Maze.RemoveWall(Current, Chosen);
		Visit(Maze.Neighbor(Current, Chosen));
	}

	// --- Braiding: knock a wall out of a share of the dead ends so the maze has loops. ---
	Maze.RefreshDeadEnds();
	TArray<FIntPoint> DeadEnds;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (Maze.CellAt(FIntPoint(X, Y)).HasFlag(EVRTMazeCellFlag::DeadEnd))
			{
				DeadEnds.Add(FIntPoint(X, Y));
			}
		}
	}
	const int32 DeadEndsBefore = DeadEnds.Num();

	// Fisher-Yates with the seeded stream keeps the result deterministic.
	for (int32 I = DeadEnds.Num() - 1; I > 0; --I)
	{
		DeadEnds.Swap(I, Random.RandRange(0, I));
	}

	const int32 ToRemove = FMath::RoundToInt(DeadEnds.Num() * FMath::Clamp(Params.BraidFraction, 0.f, 1.f));
	for (int32 I = 0; I < ToRemove; ++I)
	{
		const FIntPoint P = DeadEnds[I];
		if (Maze.OpenCount(P) != 1)
		{
			continue; // already fixed by braiding a neighbouring dead end
		}

		// Prefer a wall shared with another dead end (fixes both), else any wall to a neighbour cell.
		TArray<EVRTMazeDir, TInlineAllocator<4>> ToDeadEnd, ToAny;
		for (int32 D = 0; D < VRTMaze::DirCount; ++D)
		{
			const EVRTMazeDir Dir = static_cast<EVRTMazeDir>(D);
			const FIntPoint N = Maze.Neighbor(P, Dir);
			if (!Maze.IsInside(N) || !Maze.HasWall(P, Dir))
			{
				continue;
			}
			ToAny.Add(Dir);
			if (Maze.OpenCount(N) == 1 && !Maze.CellAt(N).HasFlag(EVRTMazeCellFlag::Room))
			{
				ToDeadEnd.Add(Dir);
			}
		}

		const TArray<EVRTMazeDir, TInlineAllocator<4>>& Pool = ToDeadEnd.Num() > 0 ? ToDeadEnd : ToAny;
		if (Pool.Num() > 0)
		{
			Maze.RemoveWall(P, Pool[Random.RandRange(0, Pool.Num() - 1)]);
		}
	}
	Maze.RefreshDeadEnds();

	// --- Distances from the spawn, and bookkeeping. ---
	const int32 Reachable = Maze.ComputeDistances(Maze.SpawnCell);

	if (OutStats)
	{
		OutStats->DeadEndsBeforeBraid = DeadEndsBefore;
		OutStats->DeadEndsAfterBraid = Maze.CountFlag(EVRTMazeCellFlag::DeadEnd);
		OutStats->RoomsCarved = Rooms.Num();
		OutStats->CellsReachable = Reachable;
		OutStats->GenerationMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
	}

	if (Reachable != Width * Height)
	{
		VRT_LOG(LogVRTMaze, Error, "Maze seed=%d: only %d of %d cells reachable from the spawn", Params.Seed, Reachable, Width * Height);
	}
	return Maze;
}
