// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
#include "FUnrealPakSettings.h"
#include "FIoStoreSettings.h"
#include "FPatchVersionDiff.h"
#include "FChunkInfo.h"
#include "FReplaceText.h"
#include "ETargetPlatform.h"
#include "FExternFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPlatformExternAssets.h"
#include "FPatcherSpecifyAsset.h"
#include "CreatePatch/HotPatcherSettingBase.h"
#include "BinariesPatchFeature.h"
#include "FPlatformBasePak.h"
#include "FPakEncryptionKeys.h"
#include "FBinariesPatchConfig.h"
#include "FHotPatcherVersion.h"
#include "FPakVersion.h"
#include "FPlatformExternAssets.h"
#include "BaseTypes/FCookShaderOptions.h"
#include "BaseTypes/FAssetRegistryOptions.h"
#include "BaseTypes.h"
// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "FExportPatchSettings.generated.h"


USTRUCT()
struct HOTPATCHERRUNTIME_API FPatherResult
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FAssetDetail> PatcherAssetDetails;
};

USTRUCT(BlueprintType)
struct FCookAdvancedOptions
{
	GENERATED_BODY()
	FCookAdvancedOptions();
	// ConcurrentSave for cooking
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCookParallelSerialize = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumberOfAssetsPerFrame = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<UClass*,int32> OverrideNumberOfAssetsPerFrame;
	FORCEINLINE const TMap<UClass*,int32>& GetOverrideNumberOfAssetsPerFrame()const{ return OverrideNumberOfAssetsPerFrame; }
	UPROPERTY(EditAnywhere)
	bool bAccompanyCookForShader = false;
};

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FExportPatchSettings:public FHotPatcherSettingBase
{
	GENERATED_USTRUCT_BODY()
public:

	FExportPatchSettings();
	virtual void Init() override;
	
	FORCEINLINE static FExportPatchSettings* Get()
	{
		static FExportPatchSettings StaticIns;

		return &StaticIns;
	}
	
	FORCEINLINE virtual TArray<FPlatformExternAssets>& GetAddExternAssetsToPlatform()override{return AddExternAssetsToPlatform;}

	FORCEINLINE bool IsAnalysisDiffAssetDependenciesOnly()const {return bAnalysisDiffAssetDependenciesOnly;}
	
	FORCEINLINE FString GetVersionId()const { return VersionId; }
	FString GetBaseVersion()const;
	FORCEINLINE TArray<FString> GetUnrealPakListOptions()const { return GetUnrealPakSettings().UnrealPakListOptions; }
	FORCEINLINE TArray<FReplaceText> GetReplacePakListTexts()const { return ReplacePakListTexts; }
	FORCEINLINE TArray<FString> GetUnrealPakCommandletOptions()const { return GetUnrealPakSettings().UnrealCommandletOptions; }
	FORCEINLINE TArray<ETargetPlatform> GetPakTargetPlatforms()const { return PakTargetPlatforms; }
	TArray<FString> GetPakTargetPlatformNames()const;
	
	FORCEINLINE bool IsSaveDiffAnalysis()const { return IsByBaseVersion() && bStorageDiffAnalysisResults; }
	FORCEINLINE TArray<FString> GetIgnoreDeletionModulesAsset()const{return IgnoreDeletionModulesAsset;}

	// FORCEINLINE bool IsPackageTracker()const { return bPackageTracker; }
	FORCEINLINE bool IsIncludeAssetRegistry()const { return bIncludeAssetRegistry; }
	FORCEINLINE bool IsIncludeGlobalShaderCache()const { return bIncludeGlobalShaderCache; }
	FORCEINLINE bool IsIncludeShaderBytecode()const { return bIncludeShaderBytecode; }
	FORCEINLINE bool IsMakeBinaryConfig()const { return bMakeBinaryConfig; }
	FORCEINLINE bool IsIncludeEngineIni()const { return bIncludeEngineIni; }
	FORCEINLINE bool IsIncludePluginIni()const { return bIncludePluginIni; }
	FORCEINLINE bool IsIncludeProjectIni()const { return bIncludeProjectIni; }

	FORCEINLINE bool IsByBaseVersion()const { return bByBaseVersion; }
	FORCEINLINE bool IsEnableExternFilesDiff()const { return bEnableExternFilesDiff; }
	
	FORCEINLINE bool IsIncludePakVersion()const { return bIncludePakVersionFile; }

	// chunk infomation
	FORCEINLINE bool IsEnableChunk()const { return bEnableChunk; }
	FORCEINLINE TArray<FChunkInfo> GetChunkInfos()const { return ChunkInfos; }

	FORCEINLINE FString GetPakVersionFileMountPoint()const { return PakVersionFileMountPoint; }
	static FPakVersion GetPakVersion(const FHotPatcherVersion& InHotPatcherVersion,const FString& InUtcTime);
	static FString GetSavePakVersionPath(const FString& InSaveAbsPath,const FHotPatcherVersion& InVersion);
	static FString GetPakCommandsSaveToPath(const FString& InSaveAbsPath, const FString& InPlatfornName, const FHotPatcherVersion& InVersion);

	FHotPatcherVersion GetNewPatchVersionInfo();
	bool GetBaseVersionInfo(FHotPatcherVersion& OutBaseVersion)const;
	FString GetCurrentVersionSavePath()const;

	FORCEINLINE bool IsCustomPakNameRegular()const {return bCustomPakNameRegular;}
	FORCEINLINE FString GetPakNameRegular()const { return PakNameRegular;}
	FORCEINLINE bool IsCustomPakPathRegular()const {return bCustomPakPathRegular;}
	FORCEINLINE FString GetPakPathRegular()const { return PakPathRegular;}
	FORCEINLINE bool IsCookPatchAssets()const {return bCookPatchAssets;}
	FORCEINLINE bool IsIgnoreDeletedAssetsInfo()const {return bIgnoreDeletedAssetsInfo;}
	FORCEINLINE bool IsSaveDeletedAssetsToNewReleaseJson()const {return bStorageDeletedAssetsToNewReleaseJson;}
	
	FORCEINLINE FIoStoreSettings GetIoStoreSettings()const { return IoStoreSettings; }
	FORCEINLINE FUnrealPakSettings GetUnrealPakSettings()const {return UnrealPakSettings;}
	FORCEINLINE TArray<FString> GetDefaultPakListOptions()const {return DefaultPakListOptions;}
	FORCEINLINE TArray<FString> GetDefaultCommandletOptions()const {return DefaultCommandletOptions;}

	FORCEINLINE bool IsCreateDefaultChunk()const { return bCreateDefaultChunk; }
	FORCEINLINE bool IsEnableMultiThread()const{ return bEnableMultiThread; }

	FORCEINLINE bool IsStorageNewRelease()const{return bStorageNewRelease;}
	FORCEINLINE bool IsStoragePakFileInfo()const{return bStoragePakFileInfo;}
	// FORCEINLINE bool IsBackupMetadata()const {return bBackupMetadata;}
	FORCEINLINE bool IsEnableProfiling()const { return bEnableProfiling; }
	
	FORCEINLINE FPakEncryptSettings GetEncryptSettings()const{ return EncryptSettings; }
	FORCEINLINE bool IsBinariesPatch()const{ return bBinariesPatch; }
	FORCEINLINE FBinariesPatchConfig GetBinariesPatchConfig()const{ return BinariesPatchConfig; }
	FORCEINLINE bool IsSharedShaderLibrary()const { return GetCookShaderOptions().bSharedShaderLibrary; }
	FORCEINLINE FCookShaderOptions GetCookShaderOptions()const {return CookShaderOptions;}
	FORCEINLINE FAssetRegistryOptions GetSerializeAssetRegistryOptions()const{return SerializeAssetRegistryOptions;}
	FORCEINLINE bool IsImportProjectSettings()const{ return bImportProjectSettings; }

	virtual FString GetCombinedAdditionalCommandletArgs()const override;
	virtual bool IsCookParallelSerialize() const { return CookAdvancedOptions.bCookParallelSerialize; }
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion")
		bool bByBaseVersion = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "BaseVersion",meta = (RelativeToGameContentDir, EditCondition="bByBaseVersion"))
		FFilePath BaseVersion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchBaseSettings")
		FString VersionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
		bool bImportProjectSettings = false;
	
	// require HDiffPatchUE plugin
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
		bool bBinariesPatch = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch", meta=(EditCondition="bBinariesPatch"))
		FBinariesPatchConfig BinariesPatchConfig;
	
	// 只对与基础包有差异的资源进行依赖分析，提高依赖分析的速度
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters",meta = (EditCondition = "!bAnalysisFilterDependencies"))
	bool bAnalysisDiffAssetDependenciesOnly = false;
	// allow tracking load asset when cooking
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
		bool bPackageTracker = true;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeAssetRegistry = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeGlobalShaderCache = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeShaderBytecode = false;

	// Only in UE5
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini Config Files")
		bool bMakeBinaryConfig = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini Config Files")
		bool bIncludeEngineIni = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini Config Files")
		bool bIncludePluginIni = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ini Config Files")
		bool bIncludeProjectIni = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "External Files")
		bool bEnableExternFilesDiff = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "External Files")
		TArray<FString> IgnoreDeletionModulesAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "External Files")
		TArray<FPlatformExternAssets> AddExternAssetsToPlatform;
	// record patch infomation to pak
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "External Files")
		bool bIncludePakVersionFile = false;
	// {
	// 	"versionId": "1.1",
	// 	"baseVersionId": "1.0",
	// 	"date": "2022.01.09-02.52.34",
	// 	"checkCode": "D13EFFEB5716F00CBB823E8E8546FB610531FE37"
	// }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "External Files",meta=(EditCondition = "bIncludePakVersionFile"))
		FString PakVersionFileMountPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk Options")
		bool bEnableChunk = false;
	
	// If the resource is not contained by any chunk, create a default chunk storage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk Options", meta = (EditCondition = "bEnableChunk"))
		bool bCreateDefaultChunk = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk Options", meta = (EditCondition = "bEnableChunk"))
		TArray<FChunkInfo> ChunkInfos;
	
	/*
	 * Cook Asset in current patch
	 * shader code gets saved inline inside material assets
	 * bShareMaterialShaderCode as false
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bCookPatchAssets = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options", meta=(EditCondition = "bCookPatchAssets"))
		FCookAdvancedOptions CookAdvancedOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options", meta=(EditCondition = "bCookPatchAssets"))
		FCookShaderOptions CookShaderOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options", meta=(EditCondition = "bCookPatchAssets"))
		FAssetRegistryOptions SerializeAssetRegistryOptions;
	// support UE4.26 later
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options", meta=(EditCondition = "!bCookPatchAssets"))
		FIoStoreSettings IoStoreSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		FUnrealPakSettings UnrealPakSettings;

	// using in Pak and IO Store
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FString> DefaultPakListOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FString> DefaultCommandletOptions;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		FPakEncryptSettings EncryptSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FReplaceText> ReplacePakListTexts;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<ETargetPlatform> PakTargetPlatforms;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bCustomPakNameRegular = false;
	// Can use value: {VERSION} {BASEVERSION} {CHUNKNAME} {PLATFORM} 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options",meta=(EditCondition = "bCustomPakNameRegular"))
		FString PakNameRegular = TEXT("{VERSION}_{CHUNKNAME}_{PLATFORM}_001_P");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bCustomPakPathRegular = false;
	// Can use value: {VERSION} {BASEVERSION} {CHUNKNAME} {PLATFORM} 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options",meta=(EditCondition = "bCustomPakPathRegular"))
		FString PakPathRegular = TEXT("{CHUNKNAME}/{PLATFORM}");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStorageNewRelease = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStoragePakFileInfo = true;
	// dont display deleted asset info in patcher
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bIgnoreDeletedAssetsInfo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStorageDeletedAssetsToNewReleaseJson = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo",meta=(EditCondition="bByBaseVersion"))
		bool bStorageDiffAnalysisResults = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStorageUnrealPakList = true;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
	// 	bool bStorageAssetDependencies = false;
	// UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "SaveTo")
	// 	bool bBackupMetadata = false;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
		bool bEnableMultiThread = false;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Advanced")
		bool bEnableProfiling = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Advanced")
		FString StorageCookedDir = TEXT("[PROJECTDIR]/Saved/Cooked");

	FString GetStorageCookedDir()const;
	FString GetChunkSavedDir(const FString& InVersionId,const FString& InBaseVersionId,const FString& InChunkName,const FString& InPlatformName)const;
	
};

struct HOTPATCHERRUNTIME_API FReplacePakRegular
{
	FReplacePakRegular()=default;
	FReplacePakRegular(const FString& InVersionId,const FString& InBaseVersionId,const FString& InChunkName,const FString& InPlatformName):
	VersionId(InVersionId),BaseVersionId(InBaseVersionId),ChunkName(InChunkName),PlatformName(InPlatformName){}
	FString VersionId;
	FString BaseVersionId;
	FString ChunkName;
	FString PlatformName;
};