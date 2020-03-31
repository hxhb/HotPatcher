#pragma once
// project header
#include "Struct/AssetManager/FAssetDependenciesInfo.h"
#include "FExternAssetFileInfo.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FPatchVersionDiff.generated.h"

USTRUCT(BlueprintType)
struct FPatchVersionDiff
{
	GENERATED_USTRUCT_BODY()
public:
	FAssetDependenciesInfo AddAssetDependInfo;
	FAssetDependenciesInfo ModifyAssetDependInfo;
	FAssetDependenciesInfo DeleteAssetDependInfo;

	TArray<FExternAssetFileInfo> AddExternalFiles;
	TArray<FExternAssetFileInfo> ModifyExternalFiles;
	TArray<FExternAssetFileInfo> DeleteExternalFiles;
};