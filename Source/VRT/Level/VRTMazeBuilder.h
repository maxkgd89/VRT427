#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Level/VRTMazeData.h"
#include "TimerManager.h"
#include "Level/VRTMazeGenerator.h"
#include "VRTMazeBuilder.generated.h"

class ANavigationData;
class APlayerStart;
class AVRTKey;
class AVRTLevelExit;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * Turns an FVRTMazeData into geometry: a floor, wall segments and corner pillars (instanced meshes, one
 * hierarchical instanced component per chunk of ChunkCells x ChunkCells cells) and a PlayerStart in the
 * spawn cell. Place one in a map; it generates and builds when the game starts, before the player spawns.
 *
 * Geometry follows plan 10.4: 4 m cells (3.8 m clear corridor between 0.2 m walls), 2 m walls, no ceiling.
 * The maze axes match the world: cell (X, Y) is at (X * CellSize, Y * CellSize) relative to this actor,
 * X grows east and Y grows south.
 */
UCLASS()
class VRT_API AVRTMazeBuilder : public AActor
{
	GENERATED_BODY()

public:
	AVRTMazeBuilder();

	/**
	 * Generates the maze for a level (1-based) and builds it. Level 1 uses Seed; every further level adds one to the
	 * seed. With bUseLevelProgression the size, rooms, braiding and bias follow VRTLevelProgression::ForLevel,
	 * otherwise the Maze* properties below are used for every level.
	 */
	void GenerateAndBuild(int32 LevelIndex = 1);

	/** Builds geometry for an existing maze (replaces what was built before). */
	void BuildMaze(const FVRTMazeData& InMaze);

	/** Removes all generated geometry and the spawned PlayerStart. */
	void ClearMaze();

	/**
	 * Checks that the navigation mesh connects the spawn to every key. Runs by itself after each rebuild; call it
	 * to repeat the check. Results go to LogVRTNav.
	 */
	void ValidateNavigation();

	/** Logs whether the navigation mesh is still building and how big its area is. */
	void LogNavigationStatus() const;

	/** True once the navigation mesh finished building after the last maze build and the path check ran. */
	bool IsNavigationReady() const { return bNavReady; }

	/** Moves a pawn to the spawn cell, standing on the floor. */
	void PlacePawnAtSpawn(APawn* Pawn) const;

	const FVRTMazeData& GetMaze() const { return Maze; }

	/** The PlayerStart in the spawn cell, or null before the first build. */
	APlayerStart* GetSpawnPoint() const { return SpawnPoint; }

	/** Increases by one every time BuildMaze runs. The dev map uses it to notice a rebuilt maze. */
	int32 GetBuildCounter() const { return BuildCounter; }

	/** Cell centre in world space at floor level, cm. */
	FVector GetCellCenterWorld(const FIntPoint& Cell) const;

	// --- Maze parameters (see FVRTMazeParams) ---

	/** The maze grows with the level (see VRTLevelProgression.h) and ignores MazeWidth/Height, rooms, braiding and bias. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRT|Maze")
	bool bUseLevelProgression = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "2", ClampMax = "64"))
	int32 MazeWidth = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "2", ClampMax = "64"))
	int32 MazeHeight = 8;

	/** Seed of level 1. Level N uses Seed + N - 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRT|Maze")
	int32 Seed = 1;

	/** Growing Tree: chance to continue from the newest cell (long corridors) instead of a random one (bushy). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NewestBias = 0.75f;

	/** Share of dead ends opened into loops (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BraidFraction = 0.2f;

	/** Open areas besides the spawn hub. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "0"))
	int32 RoomCount = 2;

	/** Spawn hub size in cells (1 = no hub). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "1", ClampMax = "4"))
	int32 SpawnHubSize = 2;

	/** Keys to place (0-4), one per quadrant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Gameplay", meta = (ClampMin = "0", ClampMax = "4"))
	int32 KeyCount = 4;

	/** A key is picked among this share (0-1) of its quadrant's cells that lie farthest from the spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Gameplay", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float KeyTopFraction = 0.25f;

	/** Minimum walking distance in cells between two keys. 0 = automatic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Gameplay", meta = (ClampMin = "0"))
	int32 MinKeyDistance = 0;

	/** Keys the exit needs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Gameplay", meta = (ClampMin = "0"))
	int32 ExitRequiredKeys = 2;

	/** Key height above the floor, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Gameplay")
	float KeyHeight = 100.f;

	/** How far the beacon stands from the cell's walls (measured to its edge), cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Gameplay", meta = (ClampMin = "0.0"))
	float BeaconCornerInset = 35.f;

	/** Place the keys (with beacons) and the exit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Gameplay")
	bool bPlaceGameplay = true;

	/** Resize the level's NavMeshBoundsVolume to the maze after every build, so the navigation mesh follows it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Navigation")
	bool bUpdateNavigation = true;

	/** Navigation area around the maze edge, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Navigation", meta = (ClampMin = "0.0"))
	float NavBoundsMargin = 400.f;

	/** Height of the navigation area, cm (from just below the floor upwards). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Navigation", meta = (ClampMin = "100.0"))
	float NavBoundsHeight = 600.f;

	/** Generate and build automatically when the game starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze")
	bool bBuildOnInit = true;

	// --- Geometry ---

	/** Distance between cell centres, cm (4 m: 3.8 m clear plus a 0.2 m wall). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Geometry", meta = (ClampMin = "100.0"))
	float CellSize = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Geometry", meta = (ClampMin = "50.0"))
	float WallHeight = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Geometry", meta = (ClampMin = "5.0"))
	float WallThickness = 20.f;

	/** Chunk edge length in cells. Every chunk has its own instanced mesh component, so it is culled on its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Geometry", meta = (ClampMin = "1"))
	int32 ChunkCells = 8;

	/** Floor margin around the maze, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Geometry", meta = (ClampMin = "0.0"))
	float FloorMargin = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Geometry")
	UMaterialInterface* WallMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze|Geometry")
	UMaterialInterface* FloorMaterial = nullptr;

protected:
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The chunk component that holds walls and pillars of the cell (created on demand). */
	UHierarchicalInstancedStaticMeshComponent* GetChunk(const FIntPoint& Cell);

	void AddBox(UHierarchicalInstancedStaticMeshComponent* Chunk, const FVector& Center, const FVector& Size) const;
	void BuildFloor();
	FVRTMazeParams MakeParams(int32 LevelIndex) const;
	void PlaceGameplayActors();

	/** Fits the NavMeshBoundsVolume to the maze and tells the navigation system. */
	void UpdateNavigation();
	UFUNCTION()
	void HandleNavigationFinished(ANavigationData* NavData);
	void PlaceSpawnPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Floor;

	UPROPERTY(Transient)
	UStaticMesh* BoxMesh = nullptr;

	UPROPERTY(Transient)
	TMap<FIntPoint, UHierarchicalInstancedStaticMeshComponent*> Chunks;

	UPROPERTY(Transient)
	APlayerStart* SpawnPoint = nullptr;

	/** Keys and the exit spawned by PlaceGameplayActors. */
	UPROPERTY(Transient)
	TArray<AActor*> GameplayActors;

	FVRTMazeData Maze;
	int32 BuildCounter = 0;

	// Navigation bookkeeping.
	bool bNavDelegateBound = false;
	FTimerHandle NavValidateTimer;
	double NavBuildStartTime = 0.0;
	bool bNavReady = false;
	bool bWarnedNoNavVolume = false;
};
