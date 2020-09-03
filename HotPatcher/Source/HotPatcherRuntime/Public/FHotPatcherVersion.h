#pragma once
// project header
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FPatcherSpecifyAsset.h"
#include "FExternAssetFileInfo.h"
#include "TargetPlatform.h"

// engine header
#include "CoreMinimal.h"

#include "BackgroundHTTP/Public/Android/AndroidPlatformBackgroundHttp.h"

#include "FHotPatcherVersion.generated.h"

USTRUCT(BlueprintType)
struct FHotPatcherPlatformFiles
{
	GENERATED_USTRUCT_BODY()
	FHotPatcherPlatformFiles()=default;
	FHotPatcherPlatformFiles(ETargetPlatform InPlatform,const TArray<FExternAssetFileInfo>& InFiles):
		Platform(InPlatform),ExternFiles(InFiles){}
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	ETargetPlatform Platform;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FExternAssetFileInfo> ExternFiles;
};

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
	TMap<ETargetPlatform,FHotPatcherPlatformFiles> ExternalFiles;
};