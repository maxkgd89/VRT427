#include "Weapons/VRTRifle.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/VRTDebugSettings.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Player/VRTGrabPointComponent.h"
#include "Player/VRTHandComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTLog.h"

namespace
{
	const TCHAR* GripStateName(EVRTGripState State)
	{
		switch (State)
		{
		case EVRTGripState::OneHand: return TEXT("OneHand");
		case EVRTGripState::TwoHand: return TEXT("TwoHand");
		default: return TEXT("None");
		}
	}
}

AVRTRifle::AVRTRifle()
{
	// Runs after the motion controllers have updated this frame. Only ticks in TwoHand.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	FireInterval = 0.1f;
	bAutomatic = true;

	// Engine cube is 100 cm wide: 40 cm along the barrel (X), 4 x 4 cm cross-section.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.40f, 0.04f, 0.04f));

	// Muzzle at the front face.
	Muzzle->SetRelativeLocation(FVector(20.f, 0.f, 0.f));

	// Held: the box centre is 12 cm ahead of the right grip, so the grip is 8 cm from the rear end.
	GripLocation = FVector(12.f, 0.f, 0.f);

	// Holstered over the shoulder: the barrel points up (+X to +Z).
	HolsterRotation = FRotator(90.f, 0.f, 0.f);

	// Left hand grab zone on the front half, 12 cm ahead of the centre (24 cm from the right grip).
	BarrelGripPoint = CreateDefaultSubobject<UVRTGrabPointComponent>(TEXT("BarrelGripPoint"));
	BarrelGripPoint->SetupAttachment(Root);
	BarrelGripPoint->SetRelativeLocation(FVector(12.f, 0.f, 0.f));
	BarrelGripPoint->PointId = FName("GunBarrel");
	BarrelGripPoint->AllowedHand = EVRTHandFilter::LeftOnly;
	BarrelGripPoint->bGrabEnabled = false;
	BarrelGripPoint->SetGrabRadius(10.f);

	static ConstructorHelpers::FObjectFinder<USoundBase> FireCue(TEXT("/Game/VRTemplate/Audio/Fire_Cue.Fire_Cue"));
	if (FireCue.Succeeded())
	{
		FireSound = FireCue.Object;
	}
}

void AVRTRifle::BeginPlay()
{
	Super::BeginPlay();

	BarrelGripPoint->OnPointGrabbed.AddDynamic(this, &AVRTRifle::HandleBarrelGrabbed);
	BarrelGripPoint->OnPointReleased.AddDynamic(this, &AVRTRifle::HandleBarrelReleased);
}

FVector AVRTRifle::ToPawnLocal(const FVector& WorldPosition) const
{
	const APawn* Pawn = GetInstigator();
	return Pawn ? Pawn->GetActorTransform().InverseTransformPosition(WorldPosition) : WorldPosition;
}

void AVRTRifle::SetGripState(EVRTGripState NewState, const TCHAR* Reason)
{
	if (NewState == GripState)
	{
		return;
	}

	const EVRTGripState OldState = GripState;
	GripState = NewState;

	const UVRTHandComponent* RightHand = GetHoldingHand();
	const UVRTHandComponent* LeftHand = BarrelGripPoint->GetHeldBy();
	const float HandDistance = (RightHand && LeftHand) ? FVector::Dist(RightHand->GetComponentLocation(), LeftHand->GetComponentLocation()) : -1.f;
	VRT_LOG(LogVRTTwoHand, Log, "%s -> %s (%s), handDistance=%.1f cm", GripStateName(OldState), GripStateName(NewState), Reason, HandDistance);

	if (NewState == EVRTGripState::TwoHand && RightHand && LeftHand)
	{
		VRT_LOG(LogVRTTwoHand, Log, "TwoHand start: rightGrip(PawnLocal)=%s leftHand(PawnLocal)=%s barrelPoint(Local to gun)=%s initialDistance=%.1f cm",
			*ToPawnLocal(RightHand->GetComponentLocation()).ToCompactString(), *ToPawnLocal(LeftHand->GetComponentLocation()).ToCompactString(),
			*BarrelGripPoint->GetRelativeLocation().ToCompactString(), HandDistance);
	}

	// The gun only needs a per-frame update while two hands aim it.
	SetActorTickEnabled(NewState == EVRTGripState::TwoHand);
	if (NewState != EVRTGripState::TwoHand)
	{
		bHasLastRotation = false;
		bWarnedClose = bWarnedUp = bWarnedJump = bWarnedStretch = false;
	}
}

void AVRTRifle::AttachToHand(UVRTHandComponent* Hand)
{
	Super::AttachToHand(Hand);
	BarrelGripPoint->bGrabEnabled = true;
	SetGripState(EVRTGripState::OneHand, TEXT("right hand drew the gun"));
}

void AVRTRifle::AttachToRightHand()
{
	if (UVRTHandComponent* RightHand = GetHoldingHand())
	{
		AttachToComponent(RightHand, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(GripLocation);
		SetActorRelativeRotation(GripRotation);
	}
}

void AVRTRifle::ReturnToHolster()
{
	// Releasing the right hand frees the left hand too and sends the gun back to the shoulder.
	BarrelGripPoint->bGrabEnabled = false;
	SetGripState(EVRTGripState::None, TEXT("right released, returning to shoulder"));
	if (UVRTHandComponent* LeftHand = BarrelGripPoint->GetHeldBy())
	{
		LeftHand->OnGripReleased();
	}
	Super::ReturnToHolster();
}

void AVRTRifle::HandleBarrelGrabbed(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand)
{
	if (GripState != EVRTGripState::OneHand)
	{
		return;
	}

	// Leave the right hand's attachment: from now on the aim line places the gun every frame.
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	LastUp = GetHoldingHand() ? GetHoldingHand()->GetUpVector() : FVector::UpVector;
	SetGripState(EVRTGripState::TwoHand, TEXT("left grabbed barrel"));
	Hand->PlayHapticPulse(0.4f, 0.08f);
}

void AVRTRifle::HandleBarrelReleased(UVRTGrabPointComponent* GrabPoint, UVRTHandComponent* Hand)
{
	if (GripState != EVRTGripState::TwoHand)
	{
		return;
	}

	SetGripState(EVRTGripState::OneHand, TEXT("left released"));
	AttachToRightHand();
}

void AVRTRifle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GripState == EVRTGripState::TwoHand)
	{
		UpdateTwoHandAim(DeltaSeconds);
	}
}

void AVRTRifle::UpdateTwoHandAim(float DeltaSeconds)
{
	UVRTHandComponent* RightHand = GetHoldingHand();
	UVRTHandComponent* LeftHand = BarrelGripPoint->GetHeldBy();
	if (!RightHand || !LeftHand)
	{
		VRT_LOG(LogVRTTwoHand, Warning, "TwoHand without both hands (right=%s left=%s), falling back to OneHand",
			*GetNameSafe(RightHand), *GetNameSafe(LeftHand));
		SetGripState(EVRTGripState::OneHand, TEXT("a hand is missing"));
		AttachToRightHand();
		return;
	}

	const FVector RightLocation = RightHand->GetComponentLocation();
	const FVector LeftLocation = LeftHand->GetComponentLocation();
	const FVector Delta = LeftLocation - RightLocation;
	const float Distance = Delta.Size();
	const FVector UpRef = RightHand->GetUpVector();

	FQuat Rotation;
	FVector Axis = FVector::ForwardVector;
	float AxisUpDot = 0.f;

	if (Distance < MinTwoHandDistance)
	{
		// The aim line is meaningless this close: follow the right controller for this frame.
		if (!bWarnedClose)
		{
			bWarnedClose = true;
			VRT_LOG(LogVRTTwoHand, Warning, "Hands %.1f cm apart (< %.1f), falling back to one-hand aim", Distance, MinTwoHandDistance);
		}
		Rotation = RightHand->GetComponentQuat() * GripRotation.Quaternion();
	}
	else
	{
		bWarnedClose = false;
		Axis = Delta / Distance;
		AxisUpDot = FVector::DotProduct(Axis, UpRef);

		FVector Up;
		if (FMath::Abs(AxisUpDot) > 0.95f)
		{
			// The controller's up is almost parallel to the aim line: roll would flip. Reuse the previous up.
			if (!bWarnedUp)
			{
				bWarnedUp = true;
				VRT_LOG(LogVRTTwoHand, Warning, "|Dot(Axis, UpRef)|=%.2f > 0.95, roll unstable, using previous up", FMath::Abs(AxisUpDot));
			}
			Up = (LastUp - Axis * FVector::DotProduct(Axis, LastUp)).GetSafeNormal();
			if (Up.IsNearlyZero())
			{
				Up = FVector::UpVector;
			}
		}
		else
		{
			bWarnedUp = false;
			Up = (UpRef - Axis * AxisUpDot).GetSafeNormal();
		}

		LastUp = Up;
		Rotation = FRotationMatrix::MakeFromXZ(Axis, Up).ToQuat();
	}

	// Position stays at the right grip.
	const FVector Location = RightLocation + Rotation.RotateVector(GripLocation);

	float AngularChangeDeg = 0.f;
	if (bHasLastRotation)
	{
		AngularChangeDeg = FMath::RadiansToDegrees(Rotation.AngularDistance(LastRotation));
		if (AngularChangeDeg > JumpWarnAngle)
		{
			if (!bWarnedJump)
			{
				bWarnedJump = true;
				VRT_LOG(LogVRTTwoHand, Warning, "Gun rotated %.1f deg in one frame (> %.1f): flip or pop", AngularChangeDeg, JumpWarnAngle);
			}
		}
		else
		{
			bWarnedJump = false;
		}
	}
	LastRotation = Rotation;
	bHasLastRotation = true;

	SetActorLocationAndRotation(Location, Rotation);

	// The left hand should sit on the barrel grip point; far away means the grab is being stretched.
	const FVector BarrelWorld = Location + Rotation.RotateVector(BarrelGripPoint->GetRelativeLocation());
	const float Drift = FVector::Dist(LeftLocation, BarrelWorld);
	if (Drift > StretchWarnDistance)
	{
		if (!bWarnedStretch)
		{
			bWarnedStretch = true;
			VRT_LOG(LogVRTTwoHand, Warning, "Left hand %.1f cm from the barrel grip point (> %.1f)", Drift, StretchWarnDistance);
		}
	}
	else
	{
		bWarnedStretch = false;
	}

	const FRotator GunRotator = Rotation.Rotator();
	VRT_LOG_THROTTLED(LogVRTTwoHand, VeryVerbose, 0.25,
		"R(PawnLocal)=%s L(PawnLocal)=%s axis(World)=%s dist=%.1f upRef(World)=%s dot=%.2f gunRot(P/Y/R)=%.1f/%.1f/%.1f dAngle=%.2f deg",
		*ToPawnLocal(RightLocation).ToCompactString(), *ToPawnLocal(LeftLocation).ToCompactString(), *Axis.ToCompactString(), Distance,
		*UpRef.ToCompactString(), AxisUpDot, GunRotator.Pitch, GunRotator.Yaw, GunRotator.Roll, AngularChangeDeg);

	if (VRTDebug::ShowTwoHand())
	{
		UWorld* World = GetWorld();
		DrawDebugSphere(World, RightLocation, 2.f, 8, FColor::Red, false, -1.f, 0, 0.3f);
		DrawDebugSphere(World, LeftLocation, 2.f, 8, FColor::Blue, false, -1.f, 0, 0.3f);
		DrawDebugLine(World, RightLocation, LeftLocation, FColor::White, false, -1.f, 0, 0.3f);
		DrawDebugSphere(World, BarrelWorld, BarrelGripPoint->GetScaledSphereRadius(), 12, FColor::Cyan, false, -1.f, 0, 0.2f);
		// Gun axes: X red (forward), Y green, Z blue.
		DrawDebugCoordinateSystem(World, Location, Rotation.Rotator(), 15.f, false, -1.f, 0, 0.4f);
		// Right controller up vector (what the gun's up is derived from).
		DrawDebugLine(World, RightLocation, RightLocation + UpRef * 15.f, FColor::Yellow, false, -1.f, 0, 0.3f);
	}
}
