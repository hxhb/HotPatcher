#pragma once
// project header
#include "FPatchVersionAssetDiff.h"
#include "FPatchVersionExternDiff.h"

// engine header
#include "CoreMinimal.h"

#include "Engine/EngineTypes.h"
#include "FPatchVersionDiff.generated.h"

USTRUCT(BlueprintType)
struct FPatchVersionDiff
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	FPatchVersionAssetDiff AssetDiffInfo;
	
	// UPROPERTY(EditAnywhere)
	// FPatchVersionExternDiff ExternDiffInfo;

	UPROPERTY(EditAnywhere)
	TMap<ETargetPlatform,FPatchVersionExternDiff> PlatformExternDiffInfo;
	
};