#pragma once

#include "Containers/Map.h"
#include "AssetManager/FAssetDependenciesDetail.h"
#include "CoreMinimal.h"
#include "FAssetDependenciesInfo.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FAssetDependenciesInfo
{
	GENERATED_USTRUCT_BODY()
	FAssetDependenciesInfo()=default;
	FAssetDependenciesInfo(const FAssetDependenciesInfo&)=default;
		//UPROPERTY(EditAnywhere, BlueprintReadWrite)
		//FString mAssetRef;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FString,FAssetDependenciesDetail> AssetsDependenciesMap;

	void AddAssetsDetail(const FAssetDetail& AssetDetail);
	bool HasAsset(const FString& InAssetPackageName)const;
	TArray<FAssetDetail> GetAssetDetails()const;
	bool GetAssetDetailByPackageName(const FString& InAssetPackageName,FAssetDetail& OutDetail)const;
	TArray<FString> GetAssetLongPackageNames()const;
	void RemoveAssetDetail(const FAssetDetail& AssetDetail);
	void RemoveAssetDetail(const FString& LongPackageName);
};

