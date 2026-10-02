#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Level/VRTMazeData.h"
#include "Level/VRTMazeGenerator.h"
#include "VRTMazeBuilder.generated.h"

class APlayerStart;
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

	/** Generates a maze from the Maze* properties below, then builds it. */
	void GenerateAndBuild();

	/** Builds geometry for an existing maze (replaces what was built before). */
	void BuildMaze(const FVRTMazeData& InMaze);

	/** Removes all generated geometry and the spawned PlayerStart. */
	void ClearMaze();

	/** Moves a pawn to the spawn cell, standing on the floor. */
	void PlacePawnAtSpawn(APawn* Pawn) const;

	const FVRTMazeData& GetMaze() const { return Maze; }

	/** Cell centre in world space at floor level, cm. */
	FVector GetCellCenterWorld(const FIntPoint& Cell) const;

	// --- Maze parameters (see FVRTMazeParams) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "2", ClampMax = "64"))
	int32 MazeWidth = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (ClampMin = "2", ClampMax = "64"))
	int32 MazeHeight = 8;

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
	void PlaceSpawnPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Maze", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Floor;

	UPROPERTY(Transient)
	UStaticMesh* BoxMesh = nullptr;

	UPROPERTY(Transient)
	TMap<FIntPoint, UHierarchicalInstancedStaticMeshComponent*> Chunks;

	UPROPERTY(Transient)
	APlayerStart* SpawnPoint = nullptr;

	FVRTMazeData Maze;
};
