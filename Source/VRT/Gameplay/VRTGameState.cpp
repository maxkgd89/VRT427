#include "Gameplay/VRTGameState.h"
#include "VRTLog.h"

void AVRTGameState::RegisterKey()
{
	++KeysTotal;
	VRT_LOG(LogVRTGameFlow, Log, "Key registered, total=%d", KeysTotal);
	OnKeysChanged.Broadcast(KeysCollected, KeysTotal);
}

void AVRTGameState::AddKey()
{
	++KeysCollected;
	VRT_LOG(LogVRTGameFlow, Log, "Key collected: %d / %d", KeysCollected, KeysTotal);
	OnKeysChanged.Broadcast(KeysCollected, KeysTotal);
}
