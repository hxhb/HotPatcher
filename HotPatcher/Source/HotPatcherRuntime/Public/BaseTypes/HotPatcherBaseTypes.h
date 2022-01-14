#pragma once
#include "ETargetPlatform.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

enum class EPatchAssetType:uint8
{
	None,
	NEW,
	MODIFY
};

using FCookActionEvent = TFunction<void(const FSoftObjectPath&,ETargetPlatform)>;


struct HOTPATCHERRUNTIME_API  FCookActionCallback
{
	FCookActionEvent BeginCookCallback = nullptr;
	FCookActionEvent PackageSavedCallback = nullptr;
	FCookActionEvent CookFailedCallback = nullptr;
};
