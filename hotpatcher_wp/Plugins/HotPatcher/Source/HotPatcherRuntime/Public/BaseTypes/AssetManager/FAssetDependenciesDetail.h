#pragma once

//project header
#include "FAssetDetail.h"

//engine header
#include "CoreMinimal.h"
#include "FAssetDependenciesDetail.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FAssetDependenciesDetail
{
	GENERATED_USTRUCT_BODY()

	FAssetDependenciesDetail() = default;
	FAssetDependenciesDetail(const FAssetDependenciesDetail&) = default;
	FAssetDependenciesDetail& operator=(const FAssetDependenciesDetail&) = default;
	FORCEINLINE FAssetDependenciesDetail(const FString& InModuleCategory, const TMap<FString, FAssetDetail>& InDependAssetDetails)
		: ModuleCategory(InModuleCategory), AssetDependencyDetails(InDependAssetDetails)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString ModuleCategory;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//	TArray<FString> mDependAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FString,FAssetDetail> AssetDependencyDetails;
};
