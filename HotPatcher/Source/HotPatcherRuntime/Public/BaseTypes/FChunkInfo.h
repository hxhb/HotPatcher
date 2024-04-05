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
#include "BaseTypes/FAssetRegistryOptions.h"

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
struct HOTPATCHERRUNTIME_API FPakInternalInfo
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
struct HOTPATCHERRUNTIME_API FPakCommand
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
	UPROPERTY(EditAnywhere)
	FString ChunkName;
	UPROPERTY(EditAnywhere)
	FString MountPath;
	UPROPERTY(EditAnywhere)
	FString AssetPackage;
	UPROPERTY(EditAnywhere)
	TArray<FString> PakCommands;
	UPROPERTY(EditAnywhere)
	TArray<FString> IoStoreCommands;

	EPatchAssetType Type = EPatchAssetType::None;
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPakFileProxy
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	FString ChunkStoreName;
	UPROPERTY(EditAnywhere)
	ETargetPlatform Platform = ETargetPlatform::None;
	UPROPERTY(EditAnywhere)
	FString StorageDirectory;
	// UPROPERTY(EditAnywhere)
	// FString PakCommandSavePath;
	// UPROPERTY(EditAnywhere)
	// FString PakSavePath;
	UPROPERTY(EditAnywhere)
	TArray<FPakCommand> PakCommands;
	UPROPERTY(EditAnywhere)
	TArray<FString> IoStoreCommands;
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FChunkInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FChunkInfo()
	{
		AssetRegistryDependencyTypes = TArray<EAssetRegistryDependencyTypeEx>{ EAssetRegistryDependencyTypeEx::Packages };
	}
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString ChunkName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		FString ChunkAliasName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		int32 Priority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bMonolithic = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="bMonolithic"))
		EMonolithicPathMode MonolithicPathMode = EMonolithicPathMode::MountPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bOutputDebugInfo = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bStorageUnrealPakList = true;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bStorageIoStorePakList = true;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shader")
		FCookShaderOptions CookShaderOptions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetRegistry")
		FAssetRegistryOptions AssetRegistryOptions;
	
	FORCEINLINE FCookShaderOptions GetCookShaderOptions()const {return CookShaderOptions;}
	FString GetShaderLibraryName() const;
	

	
	TArray<FSoftObjectPath> GetManagedAssets()const;

	TArray<FPakFileProxy>& GetPakFileProxys(){ return PakFileProxys; }
	const TArray<FPakFileProxy>& GetPakFileProxys()const { return PakFileProxys; }
private:
	TArray<FPakFileProxy> PakFileProxys;
};


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FChunkPakCommand
{
	GENERATED_USTRUCT_BODY()
public:
	TArray<FString> AsssetPakCommands;
	TArray<FString> AsssetIoStoreCommands;
	TArray<FString> ExternFilePakCommands;
	TArray<FString> InternalPakCommands;
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FChunkAssetDescribe
{
	GENERATED_USTRUCT_BODY()
public:
	FAssetDependenciesInfo Assets;
	FAssetDependenciesInfo AddAssets;
	FAssetDependenciesInfo ModifyAssets;
	TMap<ETargetPlatform,FPlatformExternFiles> AllPlatformExFiles;
	FPakInternalInfo InternalFiles; // general platform

	bool HasValidAssets()const;
	TArray<FAssetDetail> GetAssetsDetail()const;
	TArray<FName> GetAssetsStrings()const;
	TArray<FExternFileInfo> GetExFilesByPlatform(ETargetPlatform Platform)const;
	TArray<FName> GetExternalFileNames(ETargetPlatform Platform)const;
	TArray<FName> GetInternalFileNames()const;
	FChunkInfo AsChunkInfo(const FString& ChunkName);
	
	FORCEINLINE FPakInternalInfo GetInternalInfo()const{return InternalFiles;}

};


