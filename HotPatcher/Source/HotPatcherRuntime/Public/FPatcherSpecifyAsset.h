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
	bool operator==(const FPatcherSpecifyAsset& InAsset)const
	{
		bool SameAsset = (Asset == InAsset.Asset);
		bool SamebAnalysisAssetDependencies = (bAnalysisAssetDependencies == InAsset.bAnalysisAssetDependencies);
		bool SameAssetRegistryDependencyTypes = (AssetRegistryDependencyTypes == InAsset.AssetRegistryDependencyTypes);

		return SameAsset && SameAssetRegistryDependencyTypes && SameAssetRegistryDependencyTypes;
	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FSoftObjectPath Asset;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		bool bAnalysisAssetDependencies = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bAnalysisAssetDependencies"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
};
