#pragma once

#include "CoreMinimal.h"
#include "FAssetDependenciesDetail.generated.h"

USTRUCT(BlueprintType)
struct ASSETMANAGEREX_API FAssetDependenciesDetail
{
	GENERATED_USTRUCT_BODY()

	FAssetDependenciesDetail() = default;
	FAssetDependenciesDetail(const FAssetDependenciesDetail&) = default;
	FAssetDependenciesDetail& operator=(const FAssetDependenciesDetail&) = default;
	FORCEINLINE FAssetDependenciesDetail(const FString& InModuleCategory, const TArray<FString>& InDependAsset)
		: mModuleCategory(InModuleCategory), mDependAsset(InDependAsset)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mModuleCategory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> mDependAsset;
};
