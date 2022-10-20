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
using FCookActionResultEvent = TFunction<void(const FSoftObjectPath&,ETargetPlatform,ESavePackageResult)>;


struct HOTPATCHERRUNTIME_API  FCookActionCallback
{
	FCookActionEvent OnCookBegin = nullptr;
	FCookActionResultEvent OnAssetCooked = nullptr;
};
