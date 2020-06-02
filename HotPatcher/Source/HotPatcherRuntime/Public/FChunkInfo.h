#pragma once

#include "FExternAssetFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPatcherSpecifyAsset.h"
#include "Struct/AssetManager/FAssetDependenciesInfo.h"
#include "Flib/FLibAssetManageHelperEx.h"

// engine header
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FChunkInfo.generated.h"


UENUM(BlueprintType)
enum class EMonolithicPathMode :uint8
{
	MountPath,
	PackagePath
};


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
	FChunkInfo()
	{
		AssetRegistryDependencyTypes = TArray<EAssetRegistryDependencyTypeEx>{ EAssetRegistryDependencyTypeEx::Packages };
	}
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString ChunkName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bMonolithic = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bMonolithic"))
		EMonolithicPathMode MonolithicPathMode = EMonolithicPathMode::MountPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bSavePakCommands = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bAnalysisFilterDependencies = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bAnalysisFilterDependencies"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern", meta = (EditCondition = "!bMonolithic"))
		TArray<FExternAssetFileInfo> AddExternFileToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern", meta = (EditCondition = "!bMonolithic"))
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Internal", meta = (EditCondition = "!bMonolithic"))
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
struct FChunkAssetDescribe
{
	GENERATED_USTRUCT_BODY()
public:
	FAssetDependenciesInfo Assets;
	TArray<FExternAssetFileInfo> AllExFiles;
	FPakInternalInfo InternalFiles; // general platform

	FORCEINLINE TArray<FString> GetAssetsStrings()const
	{
		auto CollectStringByAssetDep = [](const FAssetDependenciesInfo& InAssetDep)
		{
			TArray<FString> AllUnselectedAssets;
			TArray<FAssetDetail> OutAssetDetails;
			UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(InAssetDep, OutAssetDetails);

			for (const auto& AssetDetail : OutAssetDetails)
			{
				AllUnselectedAssets.AddUnique(AssetDetail.mPackagePath);
			}

			return AllUnselectedAssets;
		};

		return CollectStringByAssetDep(Assets);
	}

	FORCEINLINE TArray<FString> GetExFileStrings()const
	{
		auto CollectExFilesStrings = [](const TArray<FExternAssetFileInfo>& InFiles)->TArray<FString>
		{
			TArray<FString> result;
			for (const auto& File : InFiles)
			{
				result.AddUnique(File.FilePath.FilePath);
			}
			return result;
		};
		return CollectExFilesStrings(AllExFiles);
	}
	FORCEINLINE TArray<FString> GetInternalFileStrings()const
	{
		TArray<FString> result;
		{
			if (InternalFiles.bIncludeAssetRegistry) { result.Add(TEXT("bIncludeAssetRegistry")); };
			if (InternalFiles.bIncludeGlobalShaderCache) { result.Add(TEXT("bIncludeGlobalShaderCache")); };
			if (InternalFiles.bIncludeShaderBytecode) { result.Add(TEXT("bIncludeShaderBytecode")); };
			if (InternalFiles.bIncludeEngineIni) { result.Add(TEXT("bIncludeEngineIni")); };
			if (InternalFiles.bIncludePluginIni) { result.Add(TEXT("bIncludePluginIni")); };
			if (InternalFiles.bIncludeProjectIni) { result.Add(TEXT("bIncludeProjectIni")); };
		}
		return result;
	}
};

//USTRUCT(BlueprintType)
//struct FChunkAssets
//{
//	GENERATED_USTRUCT_BODY()
//public:
//	FAssetDependenciesInfo Assets;
//	TArray<FExternAssetFileInfo> AllExFiles;
//	TArray<FExternAssetFileInfo> AllInternalFiles;
//};



struct FPakCommand
{
public:
	FPakCommand() = default;
	FPakCommand(const FString& InMountPath, const TArray<FString>& InCommands)
		:MountPath(InMountPath),PakCommands(InCommands){}

	const FString& GetMountPath()const
	{
		return MountPath;
	}

	const TArray<FString>& GetPakCommands()const
	{
		return PakCommands;
	}

public:
	FString ChunkName;
	FString MountPath;
	FString AssetPackage;
	TArray<FString> PakCommands;
};


struct FPakFileProxy
{
public:
	FString PakCommandSavePath;
	FString PakSavePath;
 	TArray<FPakCommand> PakCommands;
};