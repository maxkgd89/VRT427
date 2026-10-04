#include "Combat/VRTTargetDummy.h"
#include "Combat/VRTHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTLog.h"

AVRTTargetDummy::AVRTTargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.8f)); // engine cube is 100 cm wide -> 50 x 50 x 180 cm
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	HealthText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthText"));
	HealthText->SetupAttachment(Mesh);
	// The mesh is scaled, so the text gets the inverse scale: it ends up 5 cm in front of the front face, at the top.
	HealthText->SetRelativeLocation(FVector(0.6f, 0.f, 0.4f));
	HealthText->SetRelativeScale3D(FVector(2.f, 1.f, 0.56f));
	HealthText->SetWorldSize(18.f);
	HealthText->SetHorizontalAlignment(EHTA_Center);
	HealthText->SetVerticalAlignment(EVRTA_TextCenter);
	HealthText->SetTextRenderColor(FColor(255, 80, 80));
	HealthText->SetCastShadow(false);

	Health = CreateDefaultSubobject<UVRTHealthComponent>(TEXT("Health"));
}

void AVRTTargetDummy::BeginPlay()
{
	Super::BeginPlay();

	Health->OnDamaged.AddDynamic(this, &AVRTTargetDummy::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &AVRTTargetDummy::HandleDeath);
	RefreshText();
}

void AVRTTargetDummy::HandleDamaged(float Damage, float NewHealth, AActor* DamageCauser)
{
	RefreshText();
}

void AVRTTargetDummy::HandleDeath(AActor* DamageCauser)
{
	VRT_LOG(LogVRTCombat, Log, "%s destroyed", *GetName());
	Destroy();
}

void AVRTTargetDummy::RefreshText()
{
	HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Health->GetHealth(), Health->GetMaxHealth())));
}
