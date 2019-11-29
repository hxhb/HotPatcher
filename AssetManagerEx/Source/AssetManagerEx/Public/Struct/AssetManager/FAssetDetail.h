#pragma once

#include "CoreMinimal.h"
#include "FAssetDetail.generated.h"

USTRUCT(BlueprintType)
struct ASSETMANAGEREX_API FAssetDetail
{
	GENERATED_USTRUCT_BODY()

	FAssetDetail() = default;
	FAssetDetail(const FAssetDetail&) = default;
	FAssetDetail& operator=(const FAssetDetail&) = default;
	FORCEINLINE FAssetDetail(const FString& InAsetType,const FString& InAssetLongPackageName,const FString& InGuid)
		: mLongPackageName(InAssetLongPackageName), mAssetType(InAsetType), mGuid(InGuid){}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mLongPackageName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mAssetType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mGuid;
	
};
