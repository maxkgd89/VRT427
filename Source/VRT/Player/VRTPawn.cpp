#include "Player/VRTPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/InputComponent.h"
#include "MotionControllerComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Gameplay/VRTGameState.h"
#include "Debug/VRTDebugSettings.h"
#include "Player/VRTHandComponent.h"
#include "Player/VRTHolsterComponent.h"
#include "Weapons/VRTPistol.h"
#include "Weapons/VRTRifle.h"
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

	LeftHand = CreateDefaultSubobject<UVRTHandComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(LeftController);
	LeftHand->Hand = EControllerHand::Left;

	RightHand = CreateDefaultSubobject<UVRTHandComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(RightController);
	RightHand->Hand = EControllerHand::Right;

	BodyAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("BodyAnchor"));
	BodyAnchor->SetupAttachment(VROrigin);

	// Waist: pistol, about 0.55 x head height, 20 cm right, slightly forward.
	WaistHolster = CreateDefaultSubobject<UVRTHolsterComponent>(TEXT("WaistHolster"));
	WaistHolster->SetupAttachment(BodyAnchor);
	WaistHolster->PointId = FName("HolsterWaist");
	WaistHolster->HeightFraction = 0.55f;
	WaistHolster->HeightOffset = 8.f;
	WaistHolster->ForwardOffset = 5.f;
	WaistHolster->RightOffset = 20.f;

	// Shoulder: gun, 10 cm below head height, 15 cm right, 10 cm behind the head.
	ShoulderHolster = CreateDefaultSubobject<UVRTHolsterComponent>(TEXT("ShoulderHolster"));
	ShoulderHolster->SetupAttachment(BodyAnchor);
	ShoulderHolster->PointId = FName("HolsterShoulder");
	ShoulderHolster->HeightFraction = 1.f;
	ShoulderHolster->HeightOffset = -10.f;
	ShoulderHolster->ForwardOffset = -10.f;
	ShoulderHolster->RightOffset = 15.f;

	WaistHolster->WeaponClass = AVRTPistol::StaticClass();

	ShoulderHolster->WeaponClass = AVRTRifle::StaticClass();

	// Wrist display: small text above the left controller, turned toward the head in Tick.
	WristDisplay = CreateDefaultSubobject<UTextRenderComponent>(TEXT("WristDisplay"));
	WristDisplay->SetupAttachment(LeftController);
	WristDisplay->SetRelativeLocation(FVector(-5.f, 0.f, 9.f));
	WristDisplay->SetWorldSize(2.f);
	WristDisplay->SetHorizontalAlignment(EHTA_Center);
	WristDisplay->SetVerticalAlignment(EVRTA_TextCenter);
	WristDisplay->SetTextRenderColor(FColor(80, 220, 255));
	WristDisplay->SetCastShadow(false);
	WristDisplay->SetText(FText::FromString(TEXT("VRT")));

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

void AVRTPawn::PlayHapticPulseBothHands(float Intensity, float DurationSec)
{
	LeftHand->PlayHapticPulse(Intensity, DurationSec);
	RightHand->PlayHapticPulse(Intensity, DurationSec);
}

void AVRTPawn::BeginPlay()
{
	Super::BeginPlay();

	if (PlayMode == EVRTPlayMode::Seated)
	{
		// Seated: eye-level origin, recentered so the head starts at the origin. The origin itself sits
		// SeatedEyeHeight above the floor (see OnConstruction), so the eyes end up at that height.
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye);
		UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
	}
	else
	{
		// Rift S is inside-out tracked; use floor-level origin so the camera sits at real head height.
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	}

	// PlayerStart is usually placed at capsule height or at floor level depending on the map, so
	// don't trust it: trace down and put the tracking origin exactly on the floor.
	FHitResult Hit;
	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FVector End = Start - FVector(0.f, 0.f, 1000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VRTSpawnFloorTrace), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		const float FloorZ = VROrigin->GetComponentLocation().Z - GetFloorToOriginHeight();
		AddActorWorldOffset(FVector(0.f, 0.f, Hit.ImpactPoint.Z - FloorZ));
	}

	if (AVRTGameState* State = GetWorld()->GetGameState<AVRTGameState>())
	{
		State->OnProgressChanged.AddDynamic(this, &AVRTPawn::RefreshWristDisplay);
	}
	RefreshWristDisplay();

	VRT_LOG(LogVRTPawn, Log, "mode=%s trackingOrigin=%d floorHit=%d originZ(World)=%.1f cameraZ(World)=%.1f",
		PlayMode == EVRTPlayMode::Seated ? TEXT("Seated") : TEXT("Standing"), (int32)UHeadMountedDisplayFunctionLibrary::GetTrackingOrigin(), Hit.bBlockingHit ? 1 : 0,
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
	PlayerInputComponent->BindAction(TEXT("RecenterHMD"), IE_Pressed, this, &AVRTPawn::OnRecenter);
	PlayerInputComponent->BindAction(FireAction, IE_Pressed, this, &AVRTPawn::OnFirePressed);
	PlayerInputComponent->BindAction(FireAction, IE_Released, this, &AVRTPawn::OnFireReleased);
	PlayerInputComponent->BindAction(TEXT("GrabLeft"), IE_Pressed, this, &AVRTPawn::OnGrabLeftPressed);
	PlayerInputComponent->BindAction(TEXT("GrabLeft"), IE_Released, this, &AVRTPawn::OnGrabLeftReleased);
	PlayerInputComponent->BindAction(TEXT("GrabRight"), IE_Pressed, this, &AVRTPawn::OnGrabRightPressed);
	PlayerInputComponent->BindAction(TEXT("GrabRight"), IE_Released, this, &AVRTPawn::OnGrabRightReleased);

	VRT_LOG(LogVRTInput, Log, "Bound axes Turn, MoveX, MoveY and actions ToggleRun, GrabLeft, GrabRight, RecenterHMD, %s", *FireAction.ToString());
}

void AVRTPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Apply the editable capsule size; VROrigin sits at the capsule bottom (floor).
	Capsule->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	VROrigin->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalfHeight + GetFloorToOriginHeight()));
}

float AVRTPawn::GetFloorToOriginHeight() const
{
	return PlayMode == EVRTPlayMode::Seated ? SeatedEyeHeight : 0.f;
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
	BodyYaw += YawDegrees; // the body turns with the snap turn, no slow follow
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

void AVRTPawn::OnRecenter()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
	bBodyYawInitialized = false; // the body yaw re-aligns with the new forward on the next tick
	VRT_LOG(LogVRTPawn, Log, "HMD recentered (RecenterHMD pressed), playMode=%s", PlayMode == EVRTPlayMode::Seated ? TEXT("Seated") : TEXT("Standing"));
}

void AVRTPawn::OnToggleRun()
{
	bRunning = !bRunning;
	VRT_LOG(LogVRTPawn, Log, "Mode %s -> %s (ToggleRun pressed)", bRunning ? TEXT("Walk") : TEXT("Run"), bRunning ? TEXT("Run") : TEXT("Walk"));
}

void AVRTPawn::OnFirePressed()
{
	RightHand->OnTriggerPressed();
}

void AVRTPawn::OnFireReleased()
{
	RightHand->OnTriggerReleased();
}

void AVRTPawn::OnGrabLeftPressed()
{
	LeftHand->OnGripPressed();
}

void AVRTPawn::OnGrabLeftReleased()
{
	LeftHand->OnGripReleased();
}

void AVRTPawn::OnGrabRightPressed()
{
	RightHand->OnGripPressed();
}

void AVRTPawn::OnGrabRightReleased()
{
	RightHand->OnGripReleased();
}

void AVRTPawn::UpdateBodyAnchor()
{
	const FVector HeadWorld = Camera->GetComponentLocation();
	const float FloorZ = VROrigin->GetComponentLocation().Z - GetFloorToOriginHeight();
	const float HeadHeight = HeadWorld.Z - FloorZ;

	// Yaw only: flatten the head's forward vector. Looking straight up or down leaves the yaw undefined,
	// so the target is then simply the current body yaw.
	const FVector FlatForward = Camera->GetForwardVector().GetSafeNormal2D();
	const float HeadYaw = FlatForward.IsNearlyZero() ? BodyYaw : FlatForward.Rotation().Yaw;
	if (!bBodyYawInitialized)
	{
		BodyYaw = HeadYaw;
		bBodyYawInitialized = true;
	}

	// The body ignores head turns inside the dead zone. Beyond it, the body turns toward the head at
	// BodyYawFollowSpeed until the head is back at the edge of the zone.
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(BodyYaw, HeadYaw);
	if (FMath::Abs(DeltaYaw) > BodyYawDeadZone)
	{
		const float TargetYaw = HeadYaw - FMath::Sign(DeltaYaw) * BodyYawDeadZone;
		BodyYaw = FMath::UnwindDegrees(FMath::FixedTurn(BodyYaw, TargetYaw, BodyYawFollowSpeed * GetWorld()->GetDeltaSeconds()));
	}

	BodyAnchor->SetWorldLocationAndRotation(FVector(HeadWorld.X, HeadWorld.Y, FloorZ), FRotator(0.f, BodyYaw, 0.f));

	if (HeadHeight > 0.f && FMath::Abs(HeadHeight - LastHeadHeight) > 1.f)
	{
		VRT_LOG(LogVRTHolster, Verbose, "Head height %.1f -> %.1f cm (holsters refit)", LastHeadHeight, HeadHeight);
		LastHeadHeight = HeadHeight;
		WaistHolster->UpdateForHeadHeight(HeadHeight);
		ShoulderHolster->UpdateForHeadHeight(HeadHeight);
	}

	VRT_LOG_THROTTLED(LogVRTHolster, VeryVerbose, 0.25, "Anchor(World)=%s yaw=%.1f headHeight=%.1f",
		*BodyAnchor->GetComponentLocation().ToCompactString(), BodyYaw, HeadHeight);

	if (VRTDebug::ShowHolsters())
	{
		DrawDebugCoordinateSystem(GetWorld(), BodyAnchor->GetComponentLocation(), BodyAnchor->GetComponentRotation(), 20.f, false, -1.f, 0, 0.5f);
	}
}

void AVRTPawn::RefreshWristDisplay()
{
	const AVRTGameState* State = GetWorld() ? GetWorld()->GetGameState<AVRTGameState>() : nullptr;
	if (!State)
	{
		return;
	}

	FString Text;
	if (State->GetLevelState() == EVRTLevelState::Complete)
	{
		Text = FString::Printf(TEXT("LEVEL %d\nCOMPLETE"), State->GetLevelIndex());
	}
	else
	{
		const int32 Missing = FMath::Max(0, State->GetRequiredKeys() - State->GetKeysCollected());
		const FString ExitLine = Missing > 0 ? FString::Printf(TEXT("EXIT NEEDS %d MORE"), Missing) : FString(TEXT("EXIT OPEN"));
		Text = FString::Printf(TEXT("KEYS %d/%d\n%s\nLEVEL %d"), State->GetKeysCollected(), State->GetKeysTotal(), *ExitLine, State->GetLevelIndex());
	}
	WristDisplay->SetText(FText::FromString(Text));
	VRT_LOG(LogVRTGameFlow, Verbose, "Wrist display: %s", *Text.Replace(TEXT("\n"), TEXT(" | ")));
}

void AVRTPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ApplyMoveInput();
	UpdateBodyAnchor();

	// Text is readable from its +X side, so point +X at the head.
	WristDisplay->SetWorldRotation((Camera->GetComponentLocation() - WristDisplay->GetComponentLocation()).Rotation());

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
