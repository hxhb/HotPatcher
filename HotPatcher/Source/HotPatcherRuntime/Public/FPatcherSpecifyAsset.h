#pragma once

#include "FLibAssetManageHelperEx.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FPatcherSpecifyAsset.generated.h"

USTRUCT(BlueprintType)
struct FPatcherSpecifyAsset
{
	GENERATED_USTRUCT_BODY()

public:
	FPatcherSpecifyAsset()
	{
		AssetRegistryDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);
	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FSoftObjectPath Asset;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		bool bAnalysisAssetDependencies;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bAnalysisAssetDependencies"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
};
