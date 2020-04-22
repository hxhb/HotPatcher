#pragma once
// project header
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FPatcherSpecifyAsset.h"
#include "FExternAssetFileInfo.h"

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
	bool bIncludeHasRefAssetsOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAssetDependenciesInfo AssetInfo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FExternAssetFileInfo> ExternalFiles;
};