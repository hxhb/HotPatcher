#pragma once

#include "CoreMinimal.h"
#include "FAssetDependenciesDetail.generated.h"

USTRUCT(BlueprintType)
struct FAssetDependenciesDetail
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mModuleCategory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FString> mDependAsset;
};
