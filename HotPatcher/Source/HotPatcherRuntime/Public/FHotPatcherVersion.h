#pragma once
// project header
#include "AssetManager/FAssetDependenciesInfo.h"

// engine header
#include "CoreMinimal.h"
#include "FHotPatcherVersion.generated.h"

USTRUCT(BlueprintType)
struct FHotPatcherVersion
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString VersionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString BaseVersionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Date;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> IncludeFilter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> IgnoreFilter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAssetDependenciesInfo AssetInfo;
};