#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVRT, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTInput, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTPawn, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTHand, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTHolster, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTWeapon, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTTwoHand, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTProjectile, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTGameFlow, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTMaze, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTNav, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTDirector, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTCombat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogVRTFlat, Log, All);

// Prefixes every line with function and line. Pass a plain string literal (no TEXT()).
#define VRT_LOG(Category, Verbosity, Format, ...) \
	UE_LOG(Category, Verbosity, TEXT("[%s:%d] ") TEXT(Format), ANSI_TO_TCHAR(__FUNCTION__), __LINE__, ##__VA_ARGS__)

// For per-frame data: logs at most once per IntervalSec per call site, and only if the category is enabled.
#define VRT_LOG_THROTTLED(Category, Verbosity, IntervalSec, Format, ...) \
	do { \
		if (UE_LOG_ACTIVE(Category, Verbosity)) \
		{ \
			static double VRT_LastLogTime = -1000.0; \
			const double VRT_Now = FPlatformTime::Seconds(); \
			if (VRT_Now - VRT_LastLogTime >= (IntervalSec)) \
			{ \
				VRT_LastLogTime = VRT_Now; \
				VRT_LOG(Category, Verbosity, Format, ##__VA_ARGS__); \
			} \
		} \
	} while (0)
