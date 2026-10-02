#include "Player/VRTGrabPointComponent.h"
#include "Player/VRTHandComponent.h"
#include "VRTCollision.h"
#include "VRTLog.h"

UVRTGrabPointComponent::UVRTGrabPointComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionProfileName(VRTCollision::GrabPointProfile);
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
	bUseAttachParentBound = false;
	InitSphereRadius(GrabRadius);
}

void UVRTGrabPointComponent::OnRegister()
{
	Super::OnRegister();
	SetSphereRadius(GrabRadius);
}

void UVRTGrabPointComponent::SetGrabRadius(float NewRadius)
{
	GrabRadius = NewRadius;
	SetSphereRadius(NewRadius);
}

bool UVRTGrabPointComponent::CanBeGrabbedBy(const UVRTHandComponent* Hand) const
{
	if (!bGrabEnabled || IsHeld() || !Hand)
	{
		return false;
	}

	switch (AllowedHand)
	{
	case EVRTHandFilter::LeftOnly:
		return Hand->Hand == EControllerHand::Left;
	case EVRTHandFilter::RightOnly:
		return Hand->Hand == EControllerHand::Right;
	default:
		return true;
	}
}

void UVRTGrabPointComponent::NotifyGrabbed(UVRTHandComponent* Hand)
{
	HeldBy = Hand;
	OnPointGrabbed.Broadcast(this, Hand);
}

void UVRTGrabPointComponent::NotifyReleased(UVRTHandComponent* Hand)
{
	if (HeldBy == Hand)
	{
		HeldBy = nullptr;
	}
	OnPointReleased.Broadcast(this, Hand);
}
