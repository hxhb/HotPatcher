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
	FPatchVersionDiff()=default;
	FPatchVersionDiff(const FPatchVersionDiff&)=default;
public:
	UPROPERTY(EditAnywhere, Category="")
	FPatchVersionAssetDiff AssetDiffInfo;
	
	// UPROPERTY(EditAnywhere)
	// FPatchVersionExternDiff ExternDiffInfo;

	UPROPERTY(EditAnywhere, Category="")
	TMap<ETargetPlatform,FPatchVersionExternDiff> PlatformExternDiffInfo;
	
};
