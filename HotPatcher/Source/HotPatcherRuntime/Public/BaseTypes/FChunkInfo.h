#pragma once
#include "HotPatcherBaseTypes.h"
#include "FExternFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPatcherSpecifyAsset.h"
#include "FPlatformExternAssets.h"
#include "BaseTypes/AssetManager/FAssetDependenciesInfo.h"
#include "FlibAssetManageHelper.h"
#include "FPlatformExternFiles.h"
#include "ETargetPlatform.h"
#include "BaseTypes/FCookShaderOptions.h"

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
	FORCEINLINE bool HasValidAssets()const
	{
		return bIncludeAssetRegistry || bIncludeGlobalShaderCache || bIncludeShaderBytecode || bIncludeEngineIni || bIncludePluginIni || bIncludeProjectIni;
	}
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeAssetRegistry = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeGlobalShaderCache = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked")
		bool bIncludeShaderBytecode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludeEngineIni = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludePluginIni = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini")
		bool bIncludeProjectIni = false;
};


USTRUCT(BlueprintType)
struct FPakCommand
{
	GENERATED_USTRUCT_BODY()
public:
	FPakCommand() = default;
	FPakCommand(const FString& InMountPath, const TArray<FString>& InCommands)
		:MountPath(InMountPath),PakCommands(InCommands){}
	bool operator==(const FPakCommand& PakCmd)const
	{
		return GetMountPath() == PakCmd.GetMountPath() && GetPakCommands() == PakCmd.GetPakCommands();
	}
	
	const FString& GetMountPath()const{ return MountPath; }
	const TArray<FString>& GetPakCommands()const{ return PakCommands; }
	const TArray<FString>& GetIoStoreCommands()const{ return IoStoreCommands; }
public:
	UPROPERTY(EditAnywhere, Category="")
	FString ChunkName;
	UPROPERTY(EditAnywhere, Category="")
	FString MountPath;
	UPROPERTY(EditAnywhere, Category="")
	FString AssetPackage;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FString> PakCommands;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FString> IoStoreCommands;

	EPatchAssetType Type = EPatchAssetType::None;
};

USTRUCT(BlueprintType)
struct FPakFileProxy
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, Category="")
	FString ChunkStoreName;
	UPROPERTY(EditAnywhere, Category="")
	ETargetPlatform Platform = ETargetPlatform::None;
	UPROPERTY(EditAnywhere, Category="")
	FString StorageDirectory;
	// UPROPERTY(EditAnywhere)
	// FString PakCommandSavePath;
	// UPROPERTY(EditAnywhere)
	// FString PakSavePath;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FPakCommand> PakCommands;
	UPROPERTY(EditAnywhere, Category="")
	TArray<FString> IoStoreCommands;
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
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
		FString ChunkName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		bool bMonolithic = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bMonolithic"), Category="")
		EMonolithicPathMode MonolithicPathMode = EMonolithicPathMode::MountPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		bool bStorageUnrealPakList = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		bool bStorageIoStorePakList = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
		bool bAnalysisFilterDependencies = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bAnalysisFilterDependencies"), Category="")
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
		bool bForceSkipContent = false;
	// force exclude asset folder e.g. Exclude editor content when cooking in Project Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Assets",meta = (RelativeToGameContentDir, LongPackageName, EditCondition="bForceSkipContent"))
		TArray<FDirectoryPath> ForceSkipContentRules;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Assets",meta = (EditCondition="bForceSkipContent"))
		TArray<FSoftObjectPath> ForceSkipAssets;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Assets",meta = (EditCondition="bForceSkipContent"))
		TArray<UClass*> ForceSkipClasses;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "External")
		TArray<FPlatformExternAssets> AddExternAssetsToPlatform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Internal", meta = (EditCondition = "!bMonolithic"))
		FPakInternalInfo InternalFiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shader", meta=(EditCondition = "bCookPatchAssets"))
		FCookShaderOptions CookShaderOptions;
	
	FORCEINLINE FCookShaderOptions GetCookShaderOptions()const {return CookShaderOptions;}
	FString GetShaderLibraryName() const;
	
	TArray<FPakFileProxy> PakFileProxys;
};

FORCEINLINE FString FChunkInfo::GetShaderLibraryName() const
{
	FString ShaderLibraryName;
	switch (GetCookShaderOptions().ShaderNameRule)
	{
	case EShaderLibNameRule::CHUNK_NAME:
		{
			ShaderLibraryName = ChunkName;
			break;
		}
	case EShaderLibNameRule::PROJECT_NAME:
		{
			ShaderLibraryName = FApp::GetProjectName();
			break;
		}
	case EShaderLibNameRule::CUSTOM:
		{
			ShaderLibraryName = GetCookShaderOptions().CustomShaderName;
			break;
		}
	}
	return ShaderLibraryName;
}


USTRUCT(BlueprintType)
struct FChunkPakCommand
{
	GENERATED_USTRUCT_BODY()
public:
	TArray<FString> AsssetPakCommands;
	TArray<FString> AsssetIoStoreCommands;
	TArray<FString> ExternFilePakCommands;
	TArray<FString> InternalPakCommands;
};

USTRUCT(BlueprintType)
struct FChunkAssetDescribe
{
	GENERATED_USTRUCT_BODY()
public:
	FAssetDependenciesInfo Assets;
	FAssetDependenciesInfo AddAssets;
	FAssetDependenciesInfo ModifyAssets;
	// TArray<FExternFileInfo> AllExFiles;
	TMap<ETargetPlatform,FPlatformExternFiles> AllPlatformExFiles;
	FPakInternalInfo InternalFiles; // general platform

	FORCEINLINE bool HasValidAssets()const
	{
		bool bHasValidExFiles = false;
		for(const auto& Item:AllPlatformExFiles)
		{
			if(!!Item.Value.ExternFiles.Num())
			{
				bHasValidExFiles = true;
				break;
			}
		}
		
		return !!GetAssetsDetail().Num() || InternalFiles.HasValidAssets() || bHasValidExFiles;
	}
	
	FORCEINLINE TArray<FAssetDetail> GetAssetsDetail()const
	{
		return Assets.GetAssetDetails();
	}
	
	
	FORCEINLINE TArray<FName> GetAssetsStrings()const
	{
		TArray<FName> AllUnselectedAssets;
		const TArray<FAssetDetail>& OutAssetDetails = GetAssetsDetail();

		for (const auto& AssetDetail : OutAssetDetails)
		{
			AllUnselectedAssets.AddUnique(AssetDetail.PackagePath);
		}
		return AllUnselectedAssets;
	}

	FORCEINLINE TArray<FExternFileInfo> GetExFilesByPlatform(ETargetPlatform Platform)const
	{
		TArray<FExternFileInfo> result;
		if(AllPlatformExFiles.Contains(Platform))
		{
			result = AllPlatformExFiles.Find(Platform)->ExternFiles;
		}
		return result;
	}
	FORCEINLINE TArray<FName> GetExternalFileNames(ETargetPlatform Platform)const
	{
		TArray<FName> ExFilesResult;
 		auto CollectExFilesStrings = [](const TArray<FExternFileInfo>& InFiles)->TArray<FName>
		{
			TArray<FName> result;
			for (const auto& File : InFiles)
			{
				result.AddUnique(FName(*File.FilePath.FilePath));
			}
			return result;
		};
		if(AllPlatformExFiles.Contains(Platform))
		{
			ExFilesResult = CollectExFilesStrings(GetExFilesByPlatform(Platform));
		}
		return ExFilesResult;
	}
	FORCEINLINE FPakInternalInfo GetInternalInfo()const{return InternalFiles;}
	
	FORCEINLINE TArray<FName> GetInternalFileNames()const
	{
		TArray<FName> result;
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

	FORCEINLINE FChunkInfo AsChunkInfo(const FString& ChunkName)
	{
		FChunkInfo DefaultChunk;

		DefaultChunk.ChunkName = ChunkName;
		DefaultChunk.bMonolithic = false;
		DefaultChunk.InternalFiles = GetInternalInfo();
		DefaultChunk.bStorageUnrealPakList = true;
		DefaultChunk.bStorageIoStorePakList = true;
		for(const auto& AssetDetail:GetAssetsDetail())
		{
			FPatcherSpecifyAsset Asset;
			Asset.Asset.SetPath(AssetDetail.PackagePath.ToString());
			DefaultChunk.IncludeSpecifyAssets.AddUnique(Asset);
		}
		for(const auto& ExFiles:AllPlatformExFiles)
		{
			FPlatformExternAssets PlatformFiles;
			PlatformFiles.TargetPlatform = ExFiles.Key;
			PlatformFiles.AddExternFileToPak = ExFiles.Value.ExternFiles;
		}
		return DefaultChunk;
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


