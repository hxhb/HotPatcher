#pragma once

//project header
#include "FAssetDetail.h"

//engine header
#include "CoreMinimal.h"
#include "FAssetDependenciesDetail.generated.h"

USTRUCT(BlueprintType)
struct ASSETMANAGEREX_API FAssetDependenciesDetail
{
	GENERATED_USTRUCT_BODY()

	FAssetDependenciesDetail() = default;
	FAssetDependenciesDetail(const FAssetDependenciesDetail&) = default;
	FAssetDependenciesDetail& operator=(const FAssetDependenciesDetail&) = default;
	FORCEINLINE FAssetDependenciesDetail(const FString& InModuleCategory, const TArray<FString>& InDependAsset,const TArray<FAssetDetail>& InDependAssetDetails)
		: mModuleCategory(InModuleCategory), mDependAsset(InDependAsset), mDependAssetDetails(InDependAssetDetails)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mModuleCategory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> mDependAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FAssetDetail> mDependAssetDetails;
};
