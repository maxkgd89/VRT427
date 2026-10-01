#include "Gameplay/VRTGameState.h"
#include "VRTLog.h"

namespace
{
	const TCHAR* LevelStateName(EVRTLevelState State)
	{
		switch (State)
		{
		case EVRTLevelState::Playing: return TEXT("Playing");
		case EVRTLevelState::Complete: return TEXT("Complete");
		default: return TEXT("Start");
		}
	}
}

void AVRTGameState::RegisterKey()
{
	++KeysTotal;
	VRT_LOG(LogVRTGameFlow, Log, "Key registered, total=%d", KeysTotal);
	OnKeysChanged.Broadcast(KeysCollected, KeysTotal);
	OnProgressChanged.Broadcast();
}

void AVRTGameState::AddKey()
{
	++KeysCollected;
	VRT_LOG(LogVRTGameFlow, Log, "Key collected: %d / %d (exit needs %d)", KeysCollected, KeysTotal, RequiredKeys);
	OnKeysChanged.Broadcast(KeysCollected, KeysTotal);
	OnProgressChanged.Broadcast();
}

void AVRTGameState::SetRequiredKeys(int32 InRequiredKeys)
{
	RequiredKeys = InRequiredKeys;
	VRT_LOG(LogVRTGameFlow, Log, "Exit requires %d keys", RequiredKeys);
	OnProgressChanged.Broadcast();
}

void AVRTGameState::SetLevelIndex(int32 InLevelIndex)
{
	LevelIndex = InLevelIndex;
	OnProgressChanged.Broadcast();
}

void AVRTGameState::SetLevelState(EVRTLevelState NewState)
{
	if (NewState == LevelState)
	{
		return;
	}

	VRT_LOG(LogVRTGameFlow, Log, "Level state %s -> %s (level %d)", LevelStateName(LevelState), LevelStateName(NewState), LevelIndex);
	LevelState = NewState;
	OnProgressChanged.Broadcast();
}
