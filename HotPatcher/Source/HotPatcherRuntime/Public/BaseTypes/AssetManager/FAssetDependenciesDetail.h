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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		FString ModuleCategory;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//	TArray<FString> mDependAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		TMap<FString,FAssetDetail> AssetDependencyDetails;
};
