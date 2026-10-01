#include "Gameplay/VRTBeacon.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTLog.h"

namespace
{
	// Created by Tools/Editor/create_test_level.py. The engine's emissive material is the fallback.
	const TCHAR* BeaconMaterialPath = TEXT("/Game/VRT/Materials/M_Beacon.M_Beacon");
	const TCHAR* FallbackMaterialPath = TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial");
}

AVRTBeacon::AVRTBeacon()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only ticks while fading out

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	// Engine cylinder: 100 cm wide, 100 cm tall, centred on its origin.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}

	ApplyShape();
}

void AVRTBeacon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyShape();
}

void AVRTBeacon::ApplyShape(float RadiusScale)
{
	const float Diameter = Radius * 2.f * RadiusScale;
	Mesh->SetRelativeScale3D(FVector(Diameter / 100.f, Diameter / 100.f, Height / 100.f));
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));
}

void AVRTBeacon::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, BeaconMaterialPath, nullptr, LOAD_NoWarn);
	if (!Base)
	{
		VRT_LOG(LogVRTGameFlow, Warning, "%s not found, using the engine emissive material", BeaconMaterialPath);
		Base = LoadObject<UMaterialInterface>(nullptr, FallbackMaterialPath);
	}
	if (Base)
	{
		Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Mesh->SetMaterial(0, Material);
	}
}

void AVRTBeacon::FadeOut(float DurationSec)
{
	if (bFadingOut)
	{
		return;
	}

	bFadingOut = true;
	FadeDuration = FMath::Max(DurationSec, 0.01f);
	FadeElapsed = 0.f;
	SetActorTickEnabled(true);
	VRT_LOG(LogVRTGameFlow, Log, "%s: beacon fading out over %.1f s", *GetName(), FadeDuration);
}

void AVRTBeacon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFadingOut)
	{
		return;
	}

	FadeElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(FadeElapsed / FadeDuration, 0.f, 1.f);
	ApplyShape(1.f - Alpha);

	if (Alpha >= 1.f)
	{
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
		VRT_LOG(LogVRTGameFlow, Log, "%s: beacon off", *GetName());
	}
}
