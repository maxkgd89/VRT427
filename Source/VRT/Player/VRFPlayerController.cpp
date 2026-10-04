#include "Player/VRFPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "VRTLog.h"

AVRFPlayerController::AVRFPlayerController()
{
	// The cursor is the aim tool in VRF, so keep it visible and never capture the mouse.
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AVRFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Game + UI: keys go both to the pawn and to the UI, the mouse stays free and is not locked to the viewport.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// Fog hides the maze layout from above; turn it off, remembering the old value to restore it on the way out.
	if (IConsoleVariable* FogShowFlag = IConsoleManager::Get().FindConsoleVariable(TEXT("ShowFlag.Fog")))
	{
		PreviousFogShowFlag = FogShowFlag->GetInt();
		FogShowFlag->Set(0);
	}

	VRT_LOG(LogVRTFlat, Log, "VRF controller ready: cursor shown, game-and-UI input, fog off (was %d)", PreviousFogShowFlag);
}

void AVRFPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PreviousFogShowFlag >= 0)
	{
		if (IConsoleVariable* FogShowFlag = IConsoleManager::Get().FindConsoleVariable(TEXT("ShowFlag.Fog")))
		{
			FogShowFlag->Set(PreviousFogShowFlag);
		}
	}

	Super::EndPlay(EndPlayReason);
}
