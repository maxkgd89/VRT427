#include "Level/VRTMazeBuilder.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "Gameplay/VRTGameMode.h"
#include "Gameplay/VRTGameState.h"
#include "Gameplay/VRTKey.h"
#include "Gameplay/VRTLevelExit.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Level/VRTLevelProgression.h"
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
		// The game mode already knows the level (from the URL option) when actors initialise.
		const AVRTGameMode* GameMode = GetWorld()->GetAuthGameMode<AVRTGameMode>();
		GenerateAndBuild(GameMode ? GameMode->GetLevelIndex() : 1);
	}
}

void AVRTMazeBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bNavDelegateBound)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			NavSys->OnNavigationGenerationFinishedDelegate.RemoveDynamic(this, &AVRTMazeBuilder::HandleNavigationFinished);
		}
		bNavDelegateBound = false;
	}
	GetWorldTimerManager().ClearTimer(NavValidateTimer);
	ClearMaze();
	Super::EndPlay(EndPlayReason);
}

FVRTMazeParams AVRTMazeBuilder::MakeParams(int32 LevelIndex) const
{
	const int32 Level = FMath::Max(LevelIndex, 1);

	FVRTMazeParams Params;
	Params.Width = MazeWidth;
	Params.Height = MazeHeight;
	Params.NewestBias = NewestBias;
	Params.BraidFraction = BraidFraction;
	Params.RoomCount = RoomCount;
	if (bUseLevelProgression)
	{
		const FVRTLevelSettings Settings = VRTLevelProgression::ForLevel(Level);
		Params.Width = Settings.Size;
		Params.Height = Settings.Size;
		Params.NewestBias = Settings.NewestBias;
		Params.BraidFraction = Settings.Braid;
		Params.RoomCount = Settings.Rooms;
	}

	Params.Seed = Seed + Level - 1;
	Params.SpawnHubSize = SpawnHubSize;
	Params.KeyCount = bPlaceGameplay ? KeyCount : 0;
	Params.KeyTopFraction = KeyTopFraction;
	Params.MinKeyDistance = MinKeyDistance;
	return Params;
}

void AVRTMazeBuilder::GenerateAndBuild(int32 LevelIndex)
{
	const FVRTMazeParams Params = MakeParams(LevelIndex);

	FVRTMazeStats Stats;
	const FVRTMazeData Generated = FVRTMazeGenerator::Generate(Params, &Stats);
	VRT_LOG(LogVRTMaze, Log, "Level %d: generated seed=%d grid=%dx%d rooms=%d dead ends %d -> %d in %.2f ms", LevelIndex, Generated.Seed, Generated.Width,
		Generated.Height, Stats.RoomsCarved, Stats.DeadEndsBeforeBraid, Stats.DeadEndsAfterBraid, Stats.GenerationMs);
	BuildMaze(Generated);

	if (AVRTGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AVRTGameState>() : nullptr)
	{
		GameState->SetMazeInfo(Generated.Seed, Generated.Width, Generated.Height);
	}
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

	// Keys and the exit belong to the maze. If the game is already running, the old keys no longer count.
	const bool bHadGameplay = GameplayActors.Num() > 0;
	for (AActor* Actor : GameplayActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	GameplayActors.Reset();
	if (bHadGameplay && GetWorld() && GetWorld()->HasBegunPlay())
	{
		if (AVRTGameState* GameState = GetWorld()->GetGameState<AVRTGameState>())
		{
			GameState->ResetKeys();
		}
	}

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

	// M_MazeWall (made by Tools/Editor/setup_maze_materials.py) is the brick material with "Used with Instanced
	// Static Meshes" enabled. Without it the instanced walls fall back to the default grey material in a packaged game.
	if (UMaterialInterface* WallPreferred = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VRT/Materials/M_MazeWall.M_MazeWall"), nullptr, LOAD_NoWarn))
	{
		WallMaterial = WallPreferred;
	}

	ClearMaze();
	Maze = InMaze;
	++BuildCounter;
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
	if (bPlaceGameplay)
	{
		PlaceGameplayActors();
	}
	if (bUpdateNavigation)
	{
		UpdateNavigation();
	}

	VRT_LOG(LogVRTMaze, Log, "Built %dx%d maze: %d walls + %d pillars in %d chunks (%d cells per chunk edge), cell=%.0f cm, wall height=%.0f cm, build time %.2f ms",
		W, H, WallCount, PillarCount, Chunks.Num(), ChunkCells, CellSize, WallHeight, (FPlatformTime::Seconds() - StartTime) * 1000.0);
	for (const TPair<FIntPoint, UHierarchicalInstancedStaticMeshComponent*>& Pair : Chunks)
	{
		VRT_LOG(LogVRTMaze, Verbose, "Chunk (%d, %d): %d instances", Pair.Key.X, Pair.Key.Y, Pair.Value ? Pair.Value->GetInstanceCount() : 0);
	}
}

void AVRTMazeBuilder::PlaceGameplayActors()
{
	const FVector FloorLocation = GetActorLocation();

	// Keys: one in every cell flagged by the generator, the beacon in the corner of the cell with the most walls.
	int32 KeysSpawned = 0;
	for (int32 Y = 0; Y < Maze.Height; ++Y)
	{
		for (int32 X = 0; X < Maze.Width; ++X)
		{
			const FIntPoint Cell(X, Y);
			if (!Maze.CellAt(Cell).HasFlag(EVRTMazeCellFlag::Key))
			{
				continue;
			}

			// Corners as (east sign, south sign) with the two walls that form them.
			struct FCorner { int32 SignX; int32 SignY; EVRTMazeDir SideX; EVRTMazeDir SideY; };
			const FCorner Corners[] = {
				{ 1, 1, EVRTMazeDir::East, EVRTMazeDir::South },
				{ 1, -1, EVRTMazeDir::East, EVRTMazeDir::North },
				{ -1, -1, EVRTMazeDir::West, EVRTMazeDir::North },
				{ -1, 1, EVRTMazeDir::West, EVRTMazeDir::South } };

			int32 BestScore = -1;
			int32 BestIndex = 0;
			for (int32 I = 0; I < 4; ++I)
			{
				// Rotate the start by the cell position so ties don't always pick the same corner.
				const int32 Index = (I + X + Y) % 4;
				const int32 Score = (Maze.HasWall(Cell, Corners[Index].SideX) ? 1 : 0) + (Maze.HasWall(Cell, Corners[Index].SideY) ? 1 : 0);
				if (Score > BestScore)
				{
					BestScore = Score;
					BestIndex = Index;
				}
			}

			const float CornerDistance = CellSize * 0.5f - WallThickness * 0.5f - BeaconCornerInset;
			const FVector2D BeaconOffset(Corners[BestIndex].SignX * CornerDistance, Corners[BestIndex].SignY * CornerDistance);

			const FVector KeyLocation = GetCellCenterWorld(Cell) + FVector(0.f, 0.f, KeyHeight);
			FActorSpawnParameters Params;
			Params.Owner = this;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.bDeferConstruction = true;
			if (AVRTKey* Key = GetWorld()->SpawnActor<AVRTKey>(AVRTKey::StaticClass(), FTransform(KeyLocation), Params))
			{
				Key->SetBeaconOffset(BeaconOffset);
				Key->FinishSpawning(FTransform(KeyLocation));
				GameplayActors.Add(Key);
				++KeysSpawned;
				VRT_LOG(LogVRTMaze, Verbose, "Key at cell (%d, %d), beacon corner (%d, %d) with %d walls", X, Y, Corners[BestIndex].SignX, Corners[BestIndex].SignY, BestScore);
			}
		}
	}

	// Exit: at the spawn cell, so the player returns to where the level started.
	FActorSpawnParameters ExitParams;
	ExitParams.Owner = this;
	ExitParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ExitParams.bDeferConstruction = true;
	const FTransform ExitTransform(GetCellCenterWorld(Maze.SpawnCell) + FVector(0.f, 0.f, 85.f)); // trigger centre one body height above the floor
	if (AVRTLevelExit* Exit = GetWorld()->SpawnActor<AVRTLevelExit>(AVRTLevelExit::StaticClass(), ExitTransform, ExitParams))
	{
		Exit->SetRequiredKeys(ExitRequiredKeys);
		Exit->FinishSpawning(ExitTransform);
		GameplayActors.Add(Exit);
	}

	VRT_LOG(LogVRTMaze, Log, "Placed %d keys with beacons and the exit (needs %d keys) at spawn cell (%d, %d)", KeysSpawned, ExitRequiredKeys,
		Maze.SpawnCell.X, Maze.SpawnCell.Y);
}

void AVRTMazeBuilder::UpdateNavigation()
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		VRT_LOG(LogVRTNav, Warning, "No navigation system in this world, the maze has no navigation mesh");
		return;
	}

	ANavMeshBoundsVolume* Volume = nullptr;
	int32 VolumeCount = 0;
	for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
	{
		if (!Volume)
		{
			Volume = *It;
		}
		++VolumeCount;
	}
	if (VolumeCount > 1)
	{
		VRT_LOG(LogVRTNav, Warning, "%d NavMeshBoundsVolumes in this level, only %s is fitted to the maze. Delete the extra ones in the editor.", VolumeCount, *Volume->GetName());
	}
	if (!Volume)
	{
		if (!bWarnedNoNavVolume)
		{
			bWarnedNoNavVolume = true;
			VRT_LOG(LogVRTNav, Warning, "No NavMeshBoundsVolume in this level: place one (Place Actors > Volumes > Nav Mesh Bounds Volume), any size. The builder resizes it to the maze.");
		}
		return;
	}

	const FVector CurrentExtent = Volume->GetComponentsBoundingBox(true).GetExtent();
	if (CurrentExtent.GetMin() < 1.f)
	{
		VRT_LOG(LogVRTNav, Warning, "%s has no usable bounds (its brush is empty). Delete it and place a new Nav Mesh Bounds Volume from the Place Actors panel.", *Volume->GetName());
		return;
	}

	// Fit the volume to the maze: centred on it, from slightly below the floor up to NavBoundsHeight.
	const FVector DesiredCenter = GetActorTransform().TransformPosition(FVector(Maze.Width * CellSize * 0.5f, Maze.Height * CellSize * 0.5f, NavBoundsHeight * 0.5f - 50.f));
	const FVector DesiredExtent(Maze.Width * CellSize * 0.5f + NavBoundsMargin, Maze.Height * CellSize * 0.5f + NavBoundsMargin, NavBoundsHeight * 0.5f);

	// A volume placed in the editor is Static and cannot be moved or scaled while playing (the engine only warns).
	if (USceneComponent* VolumeRoot = Volume->GetRootComponent())
	{
		if (VolumeRoot->Mobility != EComponentMobility::Movable)
		{
			VRT_LOG(LogVRTNav, Log, "%s was Static, making it Movable so it can follow the maze", *Volume->GetName());
			VolumeRoot->SetMobility(EComponentMobility::Movable);
		}
	}

	Volume->SetActorLocation(DesiredCenter);
	Volume->SetActorScale3D(Volume->GetActorScale3D() * (DesiredExtent / CurrentExtent));

	const FBox FittedBox = Volume->GetComponentsBoundingBox(true);
	if (!FittedBox.GetExtent().Equals(DesiredExtent, 50.f) || !FittedBox.GetCenter().Equals(DesiredCenter, 50.f))
	{
		VRT_LOG(LogVRTNav, Error, "The navigation volume did not fit the maze: centre(World)=%s extent=%s, wanted centre=%s extent=%s",
			*FittedBox.GetCenter().ToCompactString(), *FittedBox.GetExtent().ToCompactString(), *DesiredCenter.ToCompactString(), *DesiredExtent.ToCompactString());
	}

	if (!bNavDelegateBound)
	{
		NavSys->OnNavigationGenerationFinishedDelegate.AddDynamic(this, &AVRTMazeBuilder::HandleNavigationFinished);
		bNavDelegateBound = true;
	}

	bNavReady = false;
	NavBuildStartTime = FPlatformTime::Seconds();
	NavSys->OnNavigationBoundsUpdated(Volume);
	VRT_LOG(LogVRTNav, Log, "Navigation rebuild requested: bounds centre(World)=%s extent=%s", *DesiredCenter.ToCompactString(), *DesiredExtent.ToCompactString());
}

void AVRTMazeBuilder::HandleNavigationFinished(ANavigationData* NavData)
{
	// Dirty areas arrive over several frames, so finishing can fire more than once. Check the paths a moment
	// after the last finish.
	VRT_LOG(LogVRTNav, Log, "Navigation generation finished %.0f ms after the rebuild was requested", (FPlatformTime::Seconds() - NavBuildStartTime) * 1000.0);
	GetWorldTimerManager().SetTimer(NavValidateTimer, this, &AVRTMazeBuilder::ValidateNavigation, 0.5f, false);
}

void AVRTMazeBuilder::LogNavigationStatus() const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		VRT_LOG(LogVRTNav, Warning, "No navigation system in this world");
		return;
	}

	VRT_LOG(LogVRTNav, Log, "Navigation: building=%d remainingTasks=%d ready=%d navData=%s", NavSys->IsNavigationBuildInProgress() ? 1 : 0,
		NavSys->GetNumRemainingBuildTasks(), bNavReady ? 1 : 0, *GetNameSafe(NavSys->GetDefaultNavDataInstance()));
	for (const FNavigationBounds& Bounds : NavSys->GetNavigationBounds())
	{
		VRT_LOG(LogVRTNav, Log, "Navigation bounds(World): min=%s max=%s", *Bounds.AreaBox.Min.ToCompactString(), *Bounds.AreaBox.Max.ToCompactString());
	}
}

void AVRTMazeBuilder::ValidateNavigation()
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys || Maze.Width <= 0)
	{
		return;
	}
	if (NavSys->IsNavigationBuildInProgress())
	{
		GetWorldTimerManager().SetTimer(NavValidateTimer, this, &AVRTMazeBuilder::ValidateNavigation, 0.5f, false);
		return;
	}

	// Walk from the spawn to every key. A failure means enemies could not reach that part of the maze.
	const FVector Start = GetCellCenterWorld(Maze.SpawnCell) + FVector(0.f, 0.f, 10.f);
	int32 Reached = 0, Failed = 0;
	for (int32 Y = 0; Y < Maze.Height; ++Y)
	{
		for (int32 X = 0; X < Maze.Width; ++X)
		{
			const FIntPoint Cell(X, Y);
			if (!Maze.CellAt(Cell).HasFlag(EVRTMazeCellFlag::Key))
			{
				continue;
			}

			const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Start, GetCellCenterWorld(Cell) + FVector(0.f, 0.f, 10.f));
			if (Path && Path->IsValid() && !Path->IsPartial())
			{
				++Reached;
				VRT_LOG(LogVRTNav, Verbose, "Path spawn -> key cell (%d, %d): ok, length %.0f cm (%d steps in the maze)", X, Y, Path->GetPathLength(), Maze.CellAt(Cell).Distance);
			}
			else
			{
				++Failed;
				VRT_LOG(LogVRTNav, Warning, "Path spawn -> key cell (%d, %d) FAILED (%s)", X, Y, Path ? (Path->IsPartial() ? TEXT("only a partial path") : TEXT("invalid path")) : TEXT("no path object"));
			}
		}
	}

	bNavReady = true;
	if (Failed == 0)
	{
		VRT_LOG(LogVRTNav, Log, "Navigation ready %.0f ms after the rebuild was requested: spawn reaches all %d keys", (FPlatformTime::Seconds() - NavBuildStartTime) * 1000.0, Reached);
	}
	else
	{
		VRT_LOG(LogVRTNav, Error, "Navigation ready, but %d of %d keys are not reachable from the spawn", Failed, Reached + Failed);
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
