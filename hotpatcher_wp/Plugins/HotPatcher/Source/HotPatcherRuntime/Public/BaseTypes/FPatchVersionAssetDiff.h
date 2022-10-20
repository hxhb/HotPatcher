#pragma once
// project header
#include "BaseTypes/AssetManager/FAssetDependenciesInfo.h"
#include "FExternFileInfo.h"

// engine header
#include "CoreMinimal.h"

#include "FPlatformExternAssets.h"
#include "Engine/EngineTypes.h"
#include "FPatchVersionAssetDiff.generated.h"

USTRUCT(BlueprintType)
struct FPatchVersionAssetDiff
{
	GENERATED_USTRUCT_BODY()
public:
	FPatchVersionAssetDiff()=default;
	FPatchVersionAssetDiff(const FPatchVersionAssetDiff&)=default;
	
	UPROPERTY(EditAnywhere)
	FAssetDependenciesInfo AddAssetDependInfo;
	UPROPERTY(EditAnywhere)
	FAssetDependenciesInfo ModifyAssetDependInfo;
	UPROPERTY(EditAnywhere)
	FAssetDependenciesInfo DeleteAssetDependInfo;
};