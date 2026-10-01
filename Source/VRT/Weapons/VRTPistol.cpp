#include "Weapons/VRTPistol.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AVRTPistol::AVRTPistol()
{
	FireInterval = 0.4f;

	// Engine cube is 100 cm wide: long axis (X) 20 cm, then 3 cm wide and 4 cm tall.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.20f, 0.03f, 0.04f));

	// Muzzle at the front face of the box.
	Muzzle->SetRelativeLocation(FVector(10.f, 0.f, 0.f));

	// Held: the rear of the box sits in the palm, the box points along the controller's forward axis.
	GripLocation = FVector(8.f, 0.f, 0.f);

	// Basic shot sound from the VR template.
	static ConstructorHelpers::FObjectFinder<USoundBase> FireCue(TEXT("/Game/VRTemplate/Audio/Fire_Cue.Fire_Cue"));
	if (FireCue.Succeeded())
	{
		FireSound = FireCue.Object;
	}
}
