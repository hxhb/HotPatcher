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
		: PackagePath(InAssetPackagePath), AssetType(InAsetType), Guid(InGuid){}

	bool operator==(const FAssetDetail& InRight)const
	{
		bool bSamePackageName = PackagePath == InRight.PackagePath;
		bool bSameAssetType = AssetType == InRight.AssetType;
		bool bSameGUID = Guid == InRight.Guid;

		return bSamePackageName && bSameAssetType && bSameGUID;
	}
	FORCEINLINE bool IsValid()const
	{
		return !PackagePath.IsNone() && !AssetType.IsNone() && !Guid.IsNone();
	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName PackagePath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName AssetType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName Guid;
	
};


