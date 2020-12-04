#pragma once
// project header
#include "Struct/AssetManager/FAssetDetail.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FHotPatcherAssetDependency.generated.h"

USTRUCT(BlueprintType)
struct FHotPatcherAssetDependency
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FAssetDetail Asset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FAssetDetail> AssetReference;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		TArray<FAssetDetail> AssetDependency;
};
