#include "VRTGameMode.h"
#include "Gameplay/VRTGameState.h"
#include "Player/VRTPawn.h"

AVRTGameMode::AVRTGameMode()
{
	DefaultPawnClass = AVRTPawn::StaticClass();
	GameStateClass = AVRTGameState::StaticClass();
}
