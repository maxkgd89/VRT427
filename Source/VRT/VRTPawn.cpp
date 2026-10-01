#include "VRTPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/InputComponent.h"
#include "MotionControllerComponent.h"
#include "UObject/ConstructorHelpers.h"

AVRTPawn::AVRTPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	SetRootComponent(VROrigin);

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
}

void AVRTPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxisKey(EKeys::OculusTouch_Right_Thumbstick_X, this, &AVRTPawn::OnSnapTurnAxis);

	PlayerInputComponent->BindAxisKey(EKeys::OculusTouch_Left_Thumbstick_Y, this, &AVRTPawn::OnMoveForward);
	PlayerInputComponent->BindAxisKey(EKeys::OculusTouch_Left_Thumbstick_X, this, &AVRTPawn::OnMoveRight);
}

void AVRTPawn::OnSnapTurnAxis(float Value)
{
	const float AbsValue = FMath::Abs(Value);
	if (bSnapTurnArmed && AbsValue >= SnapTurnActivationThreshold)
	{
		bSnapTurnArmed = false;
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
	if (!FMath::IsNearlyZero(Value))
	{
		UpdateMoveSpeed();
		AddMovementInput(GetControllerForwardFlat(), Value);
	}
}

void AVRTPawn::OnMoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		UpdateMoveSpeed();
		AddMovementInput(GetControllerRightFlat(), Value);
	}
}

void AVRTPawn::UpdateMoveSpeed()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bRunning = PC && PC->IsInputKeyDown(EKeys::OculusTouch_Left_Thumbstick_Click);
	Movement->MaxSpeed = bRunning ? RunSpeed : WalkSpeed;
}
