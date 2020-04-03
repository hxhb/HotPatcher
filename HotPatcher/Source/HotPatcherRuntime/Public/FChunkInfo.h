#pragma once

#include "FExternAssetFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPatcherSpecifyAsset.h"
#include "Struct/AssetManager/FAssetDependenciesInfo.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FChunkInfo.generated.h"

// 引擎的数据和ini等配置文件
USTRUCT(BlueprintType)
struct FPakInternalInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeAssetRegistry = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeGlobalShaderCache = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeShaderBytecode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludeEngineIni = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludePluginIni = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludeProjectIni = false;
};

USTRUCT(BlueprintType)
struct FChunkInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString ChunkName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bMonolithic = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bSavePakCommands = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern")
		TArray<FExternAssetFileInfo> AddExternFileToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern")
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Internal")
		FPakInternalInfo InternalFiles;
};

USTRUCT(BlueprintType)
struct FChunkPakCommand
{
	GENERATED_USTRUCT_BODY()
public:
	TArray<FString> AsssetPakCommands;
	TArray<FString> ExternFilePakCommands;
	TArray<FString> InternalPakCommands;
};


USTRUCT(BlueprintType)
struct FChunkAssets
{
	GENERATED_USTRUCT_BODY()
public:
	FAssetDependenciesInfo Asssets;
	TArray<FExternAssetFileInfo> AllExFiles;
};
