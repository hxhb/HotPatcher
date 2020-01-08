#pragma once

#include "AssetData.h"
// #include "Flib/FLibAssetManageHelperEx.h"

#include "CoreMinimal.h"
#include "FAssetDetail.generated.h"

USTRUCT(BlueprintType)
struct ASSETMANAGEREX_API FAssetDetail
{
	GENERATED_USTRUCT_BODY()

	FAssetDetail() = default;
	FAssetDetail(const FAssetDetail&) = default;
	FAssetDetail& operator=(const FAssetDetail&) = default;
	FORCEINLINE FAssetDetail(const FString& InAssetPackagePath, const FString& InAsetType,const FString& InGuid)
		: mPackagePath(InAssetPackagePath), mAssetType(InAsetType), mGuid(InGuid){}

	bool operator==(const FAssetDetail& InRight)const
	{
		bool bSamePackageName = mPackagePath == InRight.mPackagePath;
		bool bSameAssetType = mAssetType == InRight.mAssetType;
		bool bSameGUID = mGuid == InRight.mGuid;

		return bSamePackageName && bSameAssetType && bSameGUID;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mPackagePath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mAssetType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mGuid;
	
};


