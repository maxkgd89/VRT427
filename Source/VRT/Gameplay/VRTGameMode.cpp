#include "VRTGameMode.h"
#include "Player/VRTPawn.h"

AVRTGameMode::AVRTGameMode()
{
	DefaultPawnClass = AVRTPawn::StaticClass();
}
