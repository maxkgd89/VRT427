#include "Level/VRTMazeData.h"

FIntPoint VRTMaze::DirOffset(EVRTMazeDir Dir)
{
	switch (Dir)
	{
	case EVRTMazeDir::North: return FIntPoint(0, -1);
	case EVRTMazeDir::East: return FIntPoint(1, 0);
	case EVRTMazeDir::South: return FIntPoint(0, 1);
	default: return FIntPoint(-1, 0);
	}
}

uint8 VRTMaze::DirBit(EVRTMazeDir Dir)
{
	return static_cast<uint8>(1 << static_cast<uint8>(Dir));
}

EVRTMazeDir VRTMaze::Opposite(EVRTMazeDir Dir)
{
	return static_cast<EVRTMazeDir>((static_cast<uint8>(Dir) + 2) % DirCount);
}

const TCHAR* VRTMaze::DirName(EVRTMazeDir Dir)
{
	switch (Dir)
	{
	case EVRTMazeDir::North: return TEXT("N");
	case EVRTMazeDir::East: return TEXT("E");
	case EVRTMazeDir::South: return TEXT("S");
	default: return TEXT("W");
	}
}

void FVRTMazeData::Init(int32 InWidth, int32 InHeight, int32 InSeed)
{
	Width = InWidth;
	Height = InHeight;
	Seed = InSeed;
	SpawnCell = FIntPoint::ZeroValue;
	MaxDistance = 0;
	Cells.Reset();
	Cells.SetNum(Width * Height);
}

void FVRTMazeData::RemoveWall(const FIntPoint& P, EVRTMazeDir Dir)
{
	const FIntPoint N = Neighbor(P, Dir);
	check(IsInside(P) && IsInside(N));
	CellAt(P).Walls &= static_cast<uint8>(~VRTMaze::DirBit(Dir));
	CellAt(N).Walls &= static_cast<uint8>(~VRTMaze::DirBit(VRTMaze::Opposite(Dir)));
}

int32 FVRTMazeData::OpenCount(const FIntPoint& P) const
{
	int32 Count = 0;
	for (int32 D = 0; D < VRTMaze::DirCount; ++D)
	{
		const EVRTMazeDir Dir = static_cast<EVRTMazeDir>(D);
		if (!HasWall(P, Dir) && IsInside(Neighbor(P, Dir)))
		{
			++Count;
		}
	}
	return Count;
}

void FVRTMazeData::ComputeDistanceField(const FIntPoint& Start, TArray<int32>& OutDistances) const
{
	OutDistances.Init(INDEX_NONE, Cells.Num());
	if (!IsInside(Start))
	{
		return;
	}

	TArray<FIntPoint> Queue;
	Queue.Reserve(Cells.Num());
	Queue.Add(Start);
	OutDistances[Index(Start)] = 0;

	int32 Head = 0;
	while (Head < Queue.Num())
	{
		const FIntPoint P = Queue[Head++];
		const int32 D = OutDistances[Index(P)];
		for (int32 Dir = 0; Dir < VRTMaze::DirCount; ++Dir)
		{
			const EVRTMazeDir Direction = static_cast<EVRTMazeDir>(Dir);
			const FIntPoint N = Neighbor(P, Direction);
			if (IsInside(N) && !HasWall(P, Direction) && OutDistances[Index(N)] == INDEX_NONE)
			{
				OutDistances[Index(N)] = D + 1;
				Queue.Add(N);
			}
		}
	}
}

int32 FVRTMazeData::ComputeDistances(const FIntPoint& Start)
{
	TArray<int32> Field;
	ComputeDistanceField(Start, Field);

	int32 Reached = 0;
	MaxDistance = 0;
	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		Cells[I].Distance = Field[I];
		if (Field[I] != INDEX_NONE)
		{
			++Reached;
			MaxDistance = FMath::Max(MaxDistance, Field[I]);
		}
	}
	return Reached;
}

int32 FVRTMazeData::CountFlag(EVRTMazeCellFlag::Type Flag) const
{
	int32 Count = 0;
	for (const FVRTMazeCell& Cell : Cells)
	{
		if (Cell.HasFlag(Flag))
		{
			++Count;
		}
	}
	return Count;
}

void FVRTMazeData::RefreshDeadEnds()
{
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FIntPoint P(X, Y);
			FVRTMazeCell& Cell = CellAt(P);
			const bool bDeadEnd = !Cell.HasFlag(EVRTMazeCellFlag::Room) && OpenCount(P) == 1;
			Cell.Flags = bDeadEnd ? (Cell.Flags | EVRTMazeCellFlag::DeadEnd) : (Cell.Flags & static_cast<uint8>(~EVRTMazeCellFlag::DeadEnd));
		}
	}
}

FVector FVRTMazeData::CellCenter(const FIntPoint& P, float CellSize) const
{
	return FVector((P.X + 0.5f) * CellSize, (P.Y + 0.5f) * CellSize, 0.f);
}

TArray<FString> FVRTMazeData::ToAsciiLines() const
{
	TArray<FString> Lines;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		// Top edge of row Y.
		FString Edge;
		for (int32 X = 0; X < Width; ++X)
		{
			Edge += HasWall(FIntPoint(X, Y), EVRTMazeDir::North) ? TEXT("+--") : TEXT("+  ");
		}
		Edge += TEXT("+");
		Lines.Add(Edge);

		// Cells of row Y with their west walls.
		FString Row;
		for (int32 X = 0; X < Width; ++X)
		{
			const FIntPoint P(X, Y);
			const FVRTMazeCell& Cell = CellAt(P);
			Row += HasWall(P, EVRTMazeDir::West) ? TEXT("|") : TEXT(" ");
			if (Cell.HasFlag(EVRTMazeCellFlag::Spawn)) { Row += TEXT("S "); }
			else if (Cell.HasFlag(EVRTMazeCellFlag::Key)) { Row += TEXT("K "); }
			else if (Cell.HasFlag(EVRTMazeCellFlag::DeadEnd)) { Row += TEXT("D "); }
			else if (Cell.HasFlag(EVRTMazeCellFlag::Room)) { Row += TEXT("r "); }
			else { Row += TEXT("  "); }
		}
		Row += HasWall(FIntPoint(Width - 1, Y), EVRTMazeDir::East) ? TEXT("|") : TEXT(" ");
		Lines.Add(Row);
	}

	// Bottom edge.
	FString Bottom;
	for (int32 X = 0; X < Width; ++X)
	{
		Bottom += HasWall(FIntPoint(X, Height - 1), EVRTMazeDir::South) ? TEXT("+--") : TEXT("+  ");
	}
	Bottom += TEXT("+");
	Lines.Add(Bottom);
	return Lines;
}
