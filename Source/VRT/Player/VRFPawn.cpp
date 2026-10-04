#include "Player/VRFPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTLog.h"

AVRFPawn::AVRFPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Absolute rotation: the top-down view keeps its world orientation no matter how the pawn turns.
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(Capsule);
	CameraArm->SetUsingAbsoluteRotation(true);
	CameraArm->TargetArmLength = StartArmLength;
	CameraArm->bDoCollisionTest = false; // the camera may float over walls
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bInheritPitch = false;
	CameraArm->bInheritYaw = false;
	CameraArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);

	// Body: the engine cylinder is 100 cm across and tall, scaled to the capsule's 60 x 170 cm. Visual only.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(Capsule);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCanEverAffectNavigation(false);
	BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.7f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}

	// Nose: a 35 x 12 x 12 cm box poking out of the capsule front (+X), so the facing reads from above.
	NoseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Nose"));
	NoseMesh->SetupAttachment(Capsule);
	NoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NoseMesh->SetCanEverAffectNavigation(false);
	NoseMesh->SetRelativeScale3D(FVector(0.35f, 0.12f, 0.12f));
	NoseMesh->SetRelativeLocation(FVector(40.f, 0.f, 0.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		NoseMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AVRFPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Apply the editable capsule size and camera pose; the arm is absolute, so its relative rotation is its world one.
	Capsule->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	CameraArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.f));

	DesiredArmLength = FMath::Clamp(StartArmLength, MinArm, MaxArm);
	CameraArm->TargetArmLength = DesiredArmLength;
}

void AVRFPawn::BeginPlay()
{
	Super::BeginPlay();

	// PlayerStart height differs between maps: trace down and stand the capsule exactly on the floor.
	FHitResult Hit;
	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start - FVector(0.f, 0.f, 2000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VRFSpawnFloorTrace), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, Hit.ImpactPoint.Z + CapsuleHalfHeight));
	}

	VRT_LOG(LogVRTFlat, Log, "VRF pawn active (possessed by %s) at(World)=%s, camera height %.0f cm",
		*GetNameSafe(GetController()), *GetActorLocation().ToCompactString(), CameraArm->TargetArmLength);
}

void AVRFPawn::NotifyTeleported()
{
	Super::NotifyTeleported();

	// New level or restart: drop any speed the pawn carried over and start in walk mode.
	Movement->StopMovementImmediately();
	bRunning = false;
	Movement->MaxSpeed = WalkSpeed;
	VRT_LOG(LogVRTFlat, Verbose, "VRF pawn teleported to(World)=%s", *GetActorLocation().ToCompactString());
}

FVector AVRFPawn::GetPawnViewLocation() const
{
	const float FloorZ = GetActorLocation().Z - CapsuleHalfHeight;
	return FVector(GetActorLocation().X, GetActorLocation().Y, FloorZ + EyeHeight);
}

void AVRFPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// VRF_* names live in Config/DefaultInput.ini and are separate from the VR mappings on purpose.
	PlayerInputComponent->BindAxis(TEXT("VRF_MoveForward"), this, &AVRFPawn::OnMoveForward);
	PlayerInputComponent->BindAxis(TEXT("VRF_MoveRight"), this, &AVRFPawn::OnMoveRight);
	PlayerInputComponent->BindAxis(TEXT("VRF_Zoom"), this, &AVRFPawn::OnZoom);
	PlayerInputComponent->BindAction(TEXT("VRF_Run"), IE_Pressed, this, &AVRFPawn::OnRunPressed);
	PlayerInputComponent->BindAction(TEXT("VRF_Run"), IE_Released, this, &AVRFPawn::OnRunReleased);

	VRT_LOG(LogVRTFlat, Log, "VRF input bound: VRF_MoveForward, VRF_MoveRight, VRF_Zoom, VRF_Run");
}

FVector AVRFPawn::GetViewForwardFlat() const
{
	// The camera looks nearly straight down, so its forward axis is vertical; the on-screen "up" on the floor
	// is the camera's up axis. A horizontal camera (pitch near 0) falls back to the forward axis.
	const FVector Up = Camera->GetUpVector().GetSafeNormal2D();
	return Up.IsNearlyZero() ? Camera->GetForwardVector().GetSafeNormal2D() : Up;
}

FVector AVRFPawn::GetViewRightFlat() const
{
	return Camera->GetRightVector().GetSafeNormal2D();
}

void AVRFPawn::OnMoveForward(float Value)
{
	if (bDead || FMath::IsNearlyZero(Value))
	{
		return;
	}
	AddMovementInput(GetViewForwardFlat(), Value);
}

void AVRFPawn::OnMoveRight(float Value)
{
	if (bDead || FMath::IsNearlyZero(Value))
	{
		return;
	}
	AddMovementInput(GetViewRightFlat(), Value);
}

void AVRFPawn::OnRunPressed()
{
	bRunning = true;
	Movement->MaxSpeed = RunSpeed;
	VRT_LOG(LogVRTFlat, Log, "Mode Walk -> Run (VRF_Run held)");
}

void AVRFPawn::OnRunReleased()
{
	bRunning = false;
	Movement->MaxSpeed = WalkSpeed;
	VRT_LOG(LogVRTFlat, Log, "Mode Run -> Walk (VRF_Run released)");
}

void AVRFPawn::OnZoom(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	// Positive wheel (up) zooms in: divide the arm length by the step for each notch.
	DesiredArmLength = FMath::Clamp(DesiredArmLength / FMath::Pow(ZoomStep, Value), MinArm, MaxArm);
	VRT_LOG(LogVRTFlat, Verbose, "Zoom target %.0f cm (arm %.0f cm, range %.0f..%.0f)", DesiredArmLength, CameraArm->TargetArmLength, MinArm, MaxArm);
}

void AVRFPawn::UpdateZoom(float DeltaSeconds)
{
	CameraArm->TargetArmLength = FMath::FInterpTo(CameraArm->TargetArmLength, DesiredArmLength, DeltaSeconds, ZoomInterpSpeed);
}

void AVRFPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateZoom(DeltaSeconds);
}
