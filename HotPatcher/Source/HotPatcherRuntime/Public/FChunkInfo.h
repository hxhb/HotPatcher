#pragma once

#include "FExternAssetFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPatcherSpecifyAsset.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FChunkInfo.generated.h"

USTRUCT(BlueprintType)
struct FChunkInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString ChunkName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern")
		TArray<FExternAssetFileInfo> AddExternFileToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern")
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeAssetRegistry;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeGlobalShaderCache;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeShaderBytecode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludeEngineIni;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludePluginIni;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludeProjectIni;
};