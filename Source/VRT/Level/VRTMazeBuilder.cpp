#include "Level/VRTMazeBuilder.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/PlatformTime.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTLog.h"

AVRTMazeBuilder::AVRTMazeBuilder()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(Root);

	// Engine cube: 100 cm wide, centred, with simple box collision. Walls, pillars and the floor are scaled cubes.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BoxMesh = CubeMesh.Object;
		Floor->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BrickMaterial(TEXT("/Game/StarterContent/Materials/M_Brick_Clay_New.M_Brick_Clay_New"));
	if (BrickMaterial.Succeeded())
	{
		WallMaterial = BrickMaterial.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GravelMaterial(TEXT("/Game/StarterContent/Materials/M_Ground_Gravel.M_Ground_Gravel"));
	if (GravelMaterial.Succeeded())
	{
		FloorMaterial = GravelMaterial.Object;
	}
}

void AVRTMazeBuilder::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PostInitializeComponents runs for every actor before the player is spawned, so the PlayerStart exists
	// when the game mode looks for one.
	if (bBuildOnInit && GetWorld() && GetWorld()->IsGameWorld())
	{
		GenerateAndBuild();
	}
}

void AVRTMazeBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMaze();
	Super::EndPlay(EndPlayReason);
}

void AVRTMazeBuilder::GenerateAndBuild()
{
	FVRTMazeParams Params;
	Params.Width = MazeWidth;
	Params.Height = MazeHeight;
	Params.Seed = Seed;
	Params.NewestBias = NewestBias;
	Params.BraidFraction = BraidFraction;
	Params.RoomCount = RoomCount;
	Params.SpawnHubSize = SpawnHubSize;

	FVRTMazeStats Stats;
	const FVRTMazeData Generated = FVRTMazeGenerator::Generate(Params, &Stats);
	VRT_LOG(LogVRTMaze, Log, "Generated seed=%d grid=%dx%d rooms=%d dead ends %d -> %d in %.2f ms", Generated.Seed, Generated.Width, Generated.Height,
		Stats.RoomsCarved, Stats.DeadEndsBeforeBraid, Stats.DeadEndsAfterBraid, Stats.GenerationMs);
	BuildMaze(Generated);
}

UHierarchicalInstancedStaticMeshComponent* AVRTMazeBuilder::GetChunk(const FIntPoint& Cell)
{
	const int32 Size = FMath::Max(ChunkCells, 1);
	const FIntPoint ChunkKey(Cell.X / Size, Cell.Y / Size);
	if (UHierarchicalInstancedStaticMeshComponent** Found = Chunks.Find(ChunkKey))
	{
		return *Found;
	}

	UHierarchicalInstancedStaticMeshComponent* Chunk = NewObject<UHierarchicalInstancedStaticMeshComponent>(
		this, *FString::Printf(TEXT("Chunk_%d_%d"), ChunkKey.X, ChunkKey.Y));
	Chunk->SetStaticMesh(BoxMesh);
	if (WallMaterial)
	{
		Chunk->SetMaterial(0, WallMaterial);
	}
	Chunk->SetupAttachment(GetRootComponent());
	Chunk->SetCollisionProfileName(TEXT("BlockAll"));
	Chunk->RegisterComponent();
	Chunks.Add(ChunkKey, Chunk);
	return Chunk;
}

void AVRTMazeBuilder::AddBox(UHierarchicalInstancedStaticMeshComponent* Chunk, const FVector& Center, const FVector& Size) const
{
	// The engine cube is 100 cm wide, so the instance scale is the size in metres.
	Chunk->AddInstance(FTransform(FRotator::ZeroRotator, Center, Size / 100.f));
}

void AVRTMazeBuilder::ClearMaze()
{
	for (TPair<FIntPoint, UHierarchicalInstancedStaticMeshComponent*>& Pair : Chunks)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	Chunks.Reset();

	if (SpawnPoint)
	{
		SpawnPoint->Destroy();
		SpawnPoint = nullptr;
	}
}

void AVRTMazeBuilder::BuildFloor()
{
	const float Width = Maze.Width * CellSize;
	const float Depth = Maze.Height * CellSize;
	const float Thickness = 20.f;

	// Top surface at Z = 0 (the actor's height), margin around the maze.
	Floor->SetRelativeLocation(FVector(Width * 0.5f, Depth * 0.5f, -Thickness * 0.5f));
	Floor->SetRelativeScale3D(FVector((Width + 2.f * FloorMargin) / 100.f, (Depth + 2.f * FloorMargin) / 100.f, Thickness / 100.f));
	if (FloorMaterial)
	{
		Floor->SetMaterial(0, FloorMaterial);
	}
}

void AVRTMazeBuilder::BuildMaze(const FVRTMazeData& InMaze)
{
	const double StartTime = FPlatformTime::Seconds();
	if (!BoxMesh)
	{
		VRT_LOG(LogVRTMaze, Error, "Engine cube mesh missing, cannot build the maze");
		return;
	}

	ClearMaze();
	Maze = InMaze;
	BuildFloor();

	const int32 W = Maze.Width;
	const int32 H = Maze.Height;
	const float SegmentLength = CellSize - WallThickness; // butts against the pillars, no overlapping faces
	const float HalfHeight = WallHeight * 0.5f;

	// A wall segment sits between two grid vertices. HWall(X, Y) runs from vertex (X, Y) to (X + 1, Y),
	// VWall(X, Y) from vertex (X, Y) to (X, Y + 1). Both exist if the cell on either side has the wall.
	auto HWall = [&](int32 X, int32 Y)
	{
		if (X < 0 || X >= W || Y < 0 || Y > H) { return false; }
		return (Y < H && Maze.HasWall(FIntPoint(X, Y), EVRTMazeDir::North)) || (Y > 0 && Maze.HasWall(FIntPoint(X, Y - 1), EVRTMazeDir::South));
	};
	auto VWall = [&](int32 X, int32 Y)
	{
		if (Y < 0 || Y >= H || X < 0 || X > W) { return false; }
		return (X < W && Maze.HasWall(FIntPoint(X, Y), EVRTMazeDir::West)) || (X > 0 && Maze.HasWall(FIntPoint(X - 1, Y), EVRTMazeDir::East));
	};

	int32 WallCount = 0;
	int32 PillarCount = 0;

	for (int32 Y = 0; Y <= H; ++Y)
	{
		for (int32 X = 0; X <= W; ++X)
		{
			const FIntPoint ChunkCell(FMath::Min(X, W - 1), FMath::Min(Y, H - 1));

			if (HWall(X, Y))
			{
				AddBox(GetChunk(ChunkCell), FVector((X + 0.5f) * CellSize, Y * CellSize, HalfHeight), FVector(SegmentLength, WallThickness, WallHeight));
				++WallCount;
			}
			if (VWall(X, Y))
			{
				AddBox(GetChunk(ChunkCell), FVector(X * CellSize, (Y + 0.5f) * CellSize, HalfHeight), FVector(WallThickness, SegmentLength, WallHeight));
				++WallCount;
			}

			// A pillar closes the corner wherever at least one wall touches this vertex.
			if (HWall(X - 1, Y) || HWall(X, Y) || VWall(X, Y - 1) || VWall(X, Y))
			{
				AddBox(GetChunk(ChunkCell), FVector(X * CellSize, Y * CellSize, HalfHeight), FVector(WallThickness, WallThickness, WallHeight));
				++PillarCount;
			}
		}
	}

	PlaceSpawnPoint();

	VRT_LOG(LogVRTMaze, Log, "Built %dx%d maze: %d walls + %d pillars in %d chunks (%d cells per chunk edge), cell=%.0f cm, wall height=%.0f cm, build time %.2f ms",
		W, H, WallCount, PillarCount, Chunks.Num(), ChunkCells, CellSize, WallHeight, (FPlatformTime::Seconds() - StartTime) * 1000.0);
	for (const TPair<FIntPoint, UHierarchicalInstancedStaticMeshComponent*>& Pair : Chunks)
	{
		VRT_LOG(LogVRTMaze, Verbose, "Chunk (%d, %d): %d instances", Pair.Key.X, Pair.Key.Y, Pair.Value ? Pair.Value->GetInstanceCount() : 0);
	}
}

FVector AVRTMazeBuilder::GetCellCenterWorld(const FIntPoint& Cell) const
{
	return GetActorTransform().TransformPosition(Maze.CellCenter(Cell, CellSize));
}

void AVRTMazeBuilder::PlaceSpawnPoint()
{
	// Face an open side of the spawn cell: east first, then north, south, west.
	const EVRTMazeDir Preference[] = { EVRTMazeDir::East, EVRTMazeDir::North, EVRTMazeDir::South, EVRTMazeDir::West };
	float Yaw = 0.f;
	for (const EVRTMazeDir Dir : Preference)
	{
		const FIntPoint N = Maze.Neighbor(Maze.SpawnCell, Dir);
		if (Maze.IsInside(N) && !Maze.HasWall(Maze.SpawnCell, Dir))
		{
			const FIntPoint Offset = VRTMaze::DirOffset(Dir);
			Yaw = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Offset.Y), static_cast<float>(Offset.X)));
			break;
		}
	}

	// The pawn snaps itself onto the floor at BeginPlay, so the exact height here is not critical.
	const FVector Location = GetCellCenterWorld(Maze.SpawnCell) + FVector(0.f, 0.f, 92.f);
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnPoint = GetWorld()->SpawnActor<APlayerStart>(Location, FRotator(0.f, Yaw, 0.f), Params);
	VRT_LOG(LogVRTMaze, Log, "PlayerStart in spawn cell (%d, %d) at(World)=%s yaw=%.0f", Maze.SpawnCell.X, Maze.SpawnCell.Y, *Location.ToCompactString(), Yaw);
}

void AVRTMazeBuilder::PlacePawnAtSpawn(APawn* Pawn) const
{
	if (!Pawn || !SpawnPoint)
	{
		return;
	}

	Pawn->SetActorLocationAndRotation(SpawnPoint->GetActorLocation(), SpawnPoint->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
}
