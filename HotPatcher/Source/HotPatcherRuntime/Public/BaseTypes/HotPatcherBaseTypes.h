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

using FCookResultEvent = TFunction<void(const FSoftObjectPath&,ETargetPlatform)>;
