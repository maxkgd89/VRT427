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

int32 FVRTMazeGenerator::PlaceKeys(FVRTMazeData& Maze, const FVRTMazeParams& Params, FRandomStream& Random)
{
	const int32 KeyCount = FMath::Clamp(Params.KeyCount, 0, 4);
	if (KeyCount == 0)
	{
		return 0;
	}

	const int32 MinKeyDistance = Params.MinKeyDistance > 0 ? Params.MinKeyDistance : FMath::Max(4, (Maze.Width + Maze.Height) / 4);
	const int32 MinSpawnDistance = FMath::Max(3, Maze.MaxDistance / 3);

	// Quadrants: split the grid at its middle. Each holds the cells of one region, farthest first.
	struct FRegion
	{
		TArray<FIntPoint> Cells;     // far from the spawn (preferred)
		TArray<FIntPoint> NearCells; // used only if the quadrant has no far cell, e.g. the quadrant that holds the spawn
		int32 FarthestDistance = 0;
	};
	FRegion Regions[4];
	const int32 MidX = Maze.Width / 2;
	const int32 MidY = Maze.Height / 2;
	for (int32 Y = 0; Y < Maze.Height; ++Y)
	{
		for (int32 X = 0; X < Maze.Width; ++X)
		{
			const FIntPoint P(X, Y);
			const FVRTMazeCell& Cell = Maze.CellAt(P);
			const bool bHub = Params.SpawnHubSize >= 2 && Cell.HasFlag(EVRTMazeCellFlag::Room) && Cell.Region == 0;
			if (Cell.HasFlag(EVRTMazeCellFlag::Spawn) || bHub || Cell.Distance < 2)
			{
				continue;
			}
			FRegion& Region = Regions[(Y >= MidY ? 2 : 0) + (X >= MidX ? 1 : 0)];
			(Cell.Distance >= MinSpawnDistance ? Region.Cells : Region.NearCells).Add(P);
			Region.FarthestDistance = FMath::Max(Region.FarthestDistance, Cell.Distance);
		}
	}

	// With fewer than 4 keys, use the quadrants that reach farthest from the spawn.
	TArray<int32> Order = { 0, 1, 2, 3 };
	Order.Sort([&](int32 A, int32 B) { return Regions[A].FarthestDistance > Regions[B].FarthestDistance; });

	TArray<FIntPoint> Placed;
	TArray<TArray<int32>> PlacedFields; // walking distances from every placed key

	auto FarEnoughFromPlaced = [&](const FIntPoint& P)
	{
		for (const TArray<int32>& Field : PlacedFields)
		{
			const int32 D = Field[Maze.Index(P)];
			if (D != INDEX_NONE && D < MinKeyDistance)
			{
				return false;
			}
		}
		return true;
	};

	for (int32 OrderIndex = 0; OrderIndex < KeyCount; ++OrderIndex)
	{
		FRegion& Region = Regions[Order[OrderIndex]];
		if (Region.Cells.Num() == 0 && Region.NearCells.Num() > 0)
		{
			VRT_LOG(LogVRTMaze, Verbose, "Key %d: quadrant %d has no cell %d+ steps from the spawn, using its farthest cells", OrderIndex + 1, Order[OrderIndex], MinSpawnDistance);
			Region.Cells = Region.NearCells;
		}
		if (Region.Cells.Num() == 0)
		{
			VRT_LOG(LogVRTMaze, Warning, "Key %d: quadrant %d has no cell far enough from the spawn", OrderIndex + 1, Order[OrderIndex]);
			continue;
		}

		// Farthest first; ties keep the row-major order so the result stays deterministic.
		Region.Cells.StableSort([&](const FIntPoint& A, const FIntPoint& B) { return Maze.CellAt(A).Distance > Maze.CellAt(B).Distance; });

		const int32 PoolSize = FMath::Clamp(FMath::CeilToInt(Region.Cells.Num() * FMath::Clamp(Params.KeyTopFraction, 0.01f, 1.f)), 1, Region.Cells.Num());
		TArray<FIntPoint> Preferred, Pool;
		for (int32 I = 0; I < PoolSize; ++I)
		{
			const FIntPoint P = Region.Cells[I];
			Pool.Add(P);
			const FVRTMazeCell& Cell = Maze.CellAt(P);
			if (Cell.HasFlag(EVRTMazeCellFlag::DeadEnd) || Cell.HasFlag(EVRTMazeCellFlag::Room))
			{
				Preferred.Add(P);
			}
		}

		// Preference: dead ends and rooms in the far share, then any far cell, then any cell of the quadrant.
		auto Shuffle = [&](TArray<FIntPoint>& Array)
		{
			for (int32 I = Array.Num() - 1; I > 0; --I)
			{
				Array.Swap(I, Random.RandRange(0, I));
			}
		};
		Shuffle(Preferred);
		Shuffle(Pool);

		const TArray<FIntPoint>* Attempts[] = { &Preferred, &Pool, &Region.Cells };
		bool bFound = false;
		FIntPoint Chosen = FIntPoint::ZeroValue;
		for (const TArray<FIntPoint>* List : Attempts)
		{
			for (const FIntPoint& P : *List)
			{
				if (FarEnoughFromPlaced(P))
				{
					Chosen = P;
					bFound = true;
					break;
				}
			}
			if (bFound)
			{
				break;
			}
		}

		if (!bFound)
		{
			// Spacing cannot be met (tiny maze): take the farthest cell so every key still gets placed.
			Chosen = Region.Cells[0];
			VRT_LOG(LogVRTMaze, Warning, "Key %d: minimum key distance %d not reachable, using the farthest cell", OrderIndex + 1, MinKeyDistance);
		}

		Placed.Add(Chosen);
		Maze.CellAt(Chosen).Flags |= EVRTMazeCellFlag::Key;
		TArray<int32>& Field = PlacedFields.AddDefaulted_GetRef();
		Maze.ComputeDistanceField(Chosen, Field);
		VRT_LOG(LogVRTMaze, Log, "Key %d in quadrant %d: cell (%d, %d), %d steps from the spawn%s%s", OrderIndex + 1, Order[OrderIndex], Chosen.X, Chosen.Y,
			Maze.CellAt(Chosen).Distance, Maze.CellAt(Chosen).HasFlag(EVRTMazeCellFlag::DeadEnd) ? TEXT(", dead end") : TEXT(""),
			Maze.CellAt(Chosen).HasFlag(EVRTMazeCellFlag::Room) ? TEXT(", room") : TEXT(""));
	}

	return Placed.Num();
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

	// --- Distances from the spawn, then the keys. A separate stream keeps the keys from changing the maze layout. ---
	const int32 Reachable = Maze.ComputeDistances(Maze.SpawnCell);
	FRandomStream KeyRandom(Params.Seed * 7919 + 17);
	const int32 KeysPlaced = PlaceKeys(Maze, Params, KeyRandom);

	if (OutStats)
	{
		OutStats->DeadEndsBeforeBraid = DeadEndsBefore;
		OutStats->DeadEndsAfterBraid = Maze.CountFlag(EVRTMazeCellFlag::DeadEnd);
		OutStats->RoomsCarved = Rooms.Num();
		OutStats->KeysPlaced = KeysPlaced;
		OutStats->CellsReachable = Reachable;
		OutStats->GenerationMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
	}

	if (Reachable != Width * Height)
	{
		VRT_LOG(LogVRTMaze, Error, "Maze seed=%d: only %d of %d cells reachable from the spawn", Params.Seed, Reachable, Width * Height);
	}
	return Maze;
}
