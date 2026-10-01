#include "Player/VRTPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/InputComponent.h"
#include "MotionControllerComponent.h"
#include "VRTCollision.h"
#include "VRTLog.h"
#include "UObject/ConstructorHelpers.h"

AVRTPawn::AVRTPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	// PlayerStart may sit in or on the floor; BeginPlay snaps the pawn onto the floor afterwards.
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	Capsule->SetCollisionProfileName(VRTCollision::PlayerProfile);
	SetRootComponent(Capsule);

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	VROrigin->SetupAttachment(Capsule);
	VROrigin->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalfHeight));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VROrigin);
	LeftController->SetTrackingMotionSource(FName("Left"));

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VROrigin);
	RightController->SetTrackingMotionSource(FName("Right"));

	LeftHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHandMesh"));
	LeftHandMesh->SetupAttachment(LeftController);

	RightHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHandMesh"));
	RightHandMesh->SetupAttachment(RightController);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = WalkSpeed;
	Movement->Acceleration = 2000.f;
	Movement->Deceleration = 2000.f;

	// Engine cube is 100uu wide; scale to a 10 cm cube.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (UStaticMeshComponent* Hand : { LeftHandMesh, RightHandMesh })
	{
		if (CubeMesh.Succeeded())
		{
			Hand->SetStaticMesh(CubeMesh.Object);
		}
		Hand->SetRelativeScale3D(FVector(0.1f));
		Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AVRTPawn::BeginPlay()
{
	Super::BeginPlay();

	// Rift S is inside-out tracked; use floor-level origin so the camera sits at real head height.
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);

	// PlayerStart is usually placed at capsule height or at floor level depending on the map, so
	// don't trust it: trace down and put the tracking origin exactly on the floor.
	FHitResult Hit;
	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start - FVector(0.f, 0.f, 1000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VRTSpawnFloorTrace), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		const float OriginZ = VROrigin->GetComponentLocation().Z;
		AddActorWorldOffset(FVector(0.f, 0.f, Hit.ImpactPoint.Z - OriginZ));
	}

	VRT_LOG(LogVRTPawn, Log, "trackingOrigin=%d floorHit=%d originZ(World)=%.1f cameraZ(World)=%.1f",
		(int32)UHeadMountedDisplayFunctionLibrary::GetTrackingOrigin(), Hit.bBlockingHit ? 1 : 0,
		VROrigin->GetComponentLocation().Z, Camera->GetComponentLocation().Z);
}

void AVRTPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Named mappings live in Config/DefaultInput.ini.
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AVRTPawn::OnSnapTurnAxis);
	PlayerInputComponent->BindAxis(TEXT("MoveX"), this, &AVRTPawn::OnMoveRight);
	PlayerInputComponent->BindAxis(TEXT("MoveY"), this, &AVRTPawn::OnMoveForward);
	PlayerInputComponent->BindAction(TEXT("ToggleRun"), IE_Pressed, this, &AVRTPawn::OnToggleRun);

	VRT_LOG(LogVRTInput, Log, "Bound axes Turn, MoveX, MoveY and action ToggleRun");
}

void AVRTPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Apply the editable capsule size; VROrigin sits at the capsule bottom (floor).
	Capsule->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	VROrigin->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalfHeight));
}

void AVRTPawn::OnSnapTurnAxis(float Value)
{
	const float AbsValue = FMath::Abs(Value);
	if (bSnapTurnArmed && AbsValue >= SnapTurnActivationThreshold)
	{
		bSnapTurnArmed = false;
		VRT_LOG(LogVRTInput, Verbose, "Snap turn axis=%.2f", Value);
		SnapTurn(Value > 0.f ? SnapTurnAngle : -SnapTurnAngle);
	}
	else if (!bSnapTurnArmed && AbsValue <= SnapTurnResetThreshold)
	{
		bSnapTurnArmed = true;
	}
}

void AVRTPawn::SnapTurn(float YawDegrees)
{
	const FQuat DeltaRot(FVector::UpVector, FMath::DegreesToRadians(YawDegrees));
	const FVector Pivot = Camera->GetComponentLocation();
	const FVector NewLocation = Pivot + DeltaRot.RotateVector(GetActorLocation() - Pivot);

	SetActorLocationAndRotation(NewLocation, DeltaRot * GetActorQuat());
	VRT_LOG(LogVRTPawn, Log, "Snap turn %.0f deg, yaw(World)=%.1f", YawDegrees, GetActorRotation().Yaw);
}

FVector AVRTPawn::GetControllerForwardFlat() const
{
	return LeftController->GetForwardVector().GetSafeNormal2D();
}

FVector AVRTPawn::GetControllerRightFlat() const
{
	return LeftController->GetRightVector().GetSafeNormal2D();
}

void AVRTPawn::OnMoveForward(float Value)
{
	RawMoveInput.Y = Value;
}

void AVRTPawn::OnMoveRight(float Value)
{
	RawMoveInput.X = Value;
}

void AVRTPawn::ApplyMoveInput()
{
	// Radial dead zone: judged on the stick's total deflection so drift on one axis can't leak through.
	const float Magnitude = RawMoveInput.Size();
	if (Magnitude <= MoveDeadZone)
	{
		return;
	}

	const float Scaled = FMath::Min((Magnitude - MoveDeadZone) / (1.f - MoveDeadZone), 1.f);
	const FVector2D Input = RawMoveInput / Magnitude * Scaled;

	Movement->MaxSpeed = bRunning ? RunSpeed : WalkSpeed;
	AddMovementInput(GetControllerForwardFlat(), Input.Y);
	AddMovementInput(GetControllerRightFlat(), Input.X);
}

void AVRTPawn::OnToggleRun()
{
	bRunning = !bRunning;
	VRT_LOG(LogVRTPawn, Log, "Mode %s -> %s (ToggleRun pressed)", bRunning ? "Walk" : "Run", bRunning ? "Run" : "Walk");
}

void AVRTPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ApplyMoveInput();

	// Gravity: without it the capsule stays lifted after riding up over a low obstacle.
	VerticalVelocity += GravityZ * DeltaSeconds;
	FHitResult FallHit;
	AddActorWorldOffset(FVector(0.f, 0.f, VerticalVelocity * DeltaSeconds), true, &FallHit);
	if (FallHit.bBlockingHit)
	{
		VerticalVelocity = 0.f;
	}

	// Keep the capsule under the player's head. The sweep stops it at walls; the origin is
	// shifted back by whatever the capsule actually moved so the HMD keeps its real position.
	FVector Delta = Camera->GetComponentLocation() - Capsule->GetComponentLocation();
	Delta.Z = 0.f;
	if (Delta.IsNearlyZero())
	{
		return;
	}

	const FVector Before = Capsule->GetComponentLocation();
	AddActorWorldOffset(Delta, true);
	VROrigin->AddWorldOffset(Before - Capsule->GetComponentLocation());
}
