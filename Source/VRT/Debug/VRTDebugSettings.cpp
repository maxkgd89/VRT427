#include "Debug/VRTDebugSettings.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarShowTriggers(TEXT("VRT.Debug.Triggers"), 0, TEXT("Draw trigger volumes (0/1)."), ECVF_Cheat);
	TAutoConsoleVariable<int32> CVarShowHolsters(TEXT("VRT.Debug.Holsters"), 0, TEXT("Draw holster zones (0/1)."), ECVF_Cheat);
	TAutoConsoleVariable<int32> CVarShowTwoHand(TEXT("VRT.Debug.TwoHand"), 0, TEXT("Draw two-hand grip points and aim axes (0/1)."), ECVF_Cheat);
	TAutoConsoleVariable<int32> CVarShowGrab(TEXT("VRT.Debug.Grab"), 0, TEXT("Draw hand grab spheres (0/1)."), ECVF_Cheat);
	TAutoConsoleVariable<int32> CVarShowAimLines(TEXT("VRT.Debug.Aim"), 0, TEXT("Draw weapon aim lines (0/1)."), ECVF_Cheat);
}

bool VRTDebug::ShowTriggers() { return CVarShowTriggers.GetValueOnGameThread() != 0; }
bool VRTDebug::ShowHolsters() { return CVarShowHolsters.GetValueOnGameThread() != 0; }
bool VRTDebug::ShowTwoHand() { return CVarShowTwoHand.GetValueOnGameThread() != 0; }
bool VRTDebug::ShowGrab() { return CVarShowGrab.GetValueOnGameThread() != 0; }
bool VRTDebug::ShowAimLines() { return CVarShowAimLines.GetValueOnGameThread() != 0; }
