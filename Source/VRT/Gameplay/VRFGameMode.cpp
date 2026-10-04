#include "Gameplay/VRFGameMode.h"
#include "Player/VRFPawn.h"
#include "Player/VRFPlayerController.h"
#include "UI/VRFHUD.h"

AVRFGameMode::AVRFGameMode()
{
	// Everything else (level flow, maze regeneration, death restart, GameStateClass) is inherited from AVRTGameMode.
	DefaultPawnClass = AVRFPawn::StaticClass();
	PlayerControllerClass = AVRFPlayerController::StaticClass();
	HUDClass = AVRFHUD::StaticClass();
}
