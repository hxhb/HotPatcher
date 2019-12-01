#pragma once
// project header
#include "AssetManager/FAssetDependenciesInfo.h"

// engine header
#include "CoreMinimal.h"
#include "FHotPatcherVersion.generated.h"

USTRUCT(BlueprintType)
struct FHotPatcherVersion
{
	GENERATED_USTRUCT_BODY()

public:

	FString VersionId;
	FString BaseVersionId;
	FString Date;
	FAssetDependenciesInfo AssetInfo;
};