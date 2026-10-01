#include "Debug/VRTDebugGrabTarget.h"
#include "Components/StaticMeshComponent.h"
#include "Player/VRTGrabPointComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine sphere is 100 cm wide: 0.1 -> 10 cm ball, 0.15 -> 15 cm while held.
	constexpr float IdleScale = 0.1f;
	constexpr float HeldScale = 0.15f;
}

AVRTDebugGrabTarget::AVRTDebugGrabTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	GrabPoint = CreateDefaultSubobject<UVRTGrabPointComponent>(TEXT("GrabPoint"));
	GrabPoint->PointId = FName("DebugTarget");
	SetRootComponent(GrabPoint);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GrabPoint);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(IdleScale));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
}

void AVRTDebugGrabTarget::BeginPlay()
{
	Super::BeginPlay();

	GrabPoint->OnPointGrabbed.AddDynamic(this, &AVRTDebugGrabTarget::HandleGrabbed);
	GrabPoint->OnPointReleased.AddDynamic(this, &AVRTDebugGrabTarget::HandleReleased);
}

void AVRTDebugGrabTarget::HandleGrabbed(UVRTGrabPointComponent* /*InGrabPoint*/, UVRTHandComponent* /*Hand*/)
{
	Mesh->SetRelativeScale3D(FVector(HeldScale));
}

void AVRTDebugGrabTarget::HandleReleased(UVRTGrabPointComponent* /*InGrabPoint*/, UVRTHandComponent* /*Hand*/)
{
	Mesh->SetRelativeScale3D(FVector(IdleScale));
}
