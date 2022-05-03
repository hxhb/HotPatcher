#pragma once
// project header
#include "BaseTypes/AssetManager/FAssetDetail.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FHotPatcherAssetDependency.generated.h"

USTRUCT(BlueprintType)
struct FHotPatcherAssetDependency
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		FAssetDetail Asset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		TArray<FAssetDetail> AssetReference;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		TArray<FAssetDetail> AssetDependency;
};
