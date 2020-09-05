#pragma once
// project header
#include "Struct/AssetManager/FAssetDependenciesInfo.h"
#include "FExternAssetFileInfo.h"

// engine header
#include "CoreMinimal.h"

#include "FPlatformExternAssets.h"
#include "Engine/EngineTypes.h"
#include "FPatchVersionExternDiff.generated.h"

USTRUCT(BlueprintType)
struct FPatchVersionExternDiff
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	ETargetPlatform Platform;
	UPROPERTY(EditAnywhere)
	TArray<FExternAssetFileInfo> AddExternalFiles;
	UPROPERTY(EditAnywhere)
	TArray<FExternAssetFileInfo> ModifyExternalFiles;
	UPROPERTY(EditAnywhere)
	TArray<FExternAssetFileInfo> DeleteExternalFiles;
};