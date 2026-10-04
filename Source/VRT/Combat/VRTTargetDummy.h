#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRTTargetDummy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UVRTHealthComponent;

/**
 * Shooting target with health, for testing damage (step 11). Shows its health as text on the side that faces forward,
 * and removes itself when it dies. Spawned in front of the player by the console command VRT.Dummy.Spawn.
 */
UCLASS()
class VRT_API AVRTTargetDummy : public AActor
{
	GENERATED_BODY()

public:
	AVRTTargetDummy();

	UVRTHealthComponent* GetHealthComponent() const { return Health; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDamaged(float Damage, float NewHealth, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AActor* DamageCauser);

	void RefreshText();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Combat")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Combat")
	UTextRenderComponent* HealthText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRT|Combat")
	UVRTHealthComponent* Health;
};
