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
#include "FlibPatchParserHelper.h"
#include "CreatePatch/HotPatcherSettingBase.h"
#include "BinariesPatchFeature.h"
#include "FPlatformBasePak.h"

// engine header
#if WITH_PACKAGE_CONTEXT
	#include "UObject/SavePackage.h"
	#include "Serialization/BulkDataManifest.h"
#endif

#include "CoreMinimal.h"

#include "FBinariesPatchConfig.h"
#include "FPlatformExternAssets.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "FExportPatchSettings.generated.h"


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPakEncryptSettings
{
	GENERATED_BODY()
	// -encrypt
	UPROPERTY(EditAnywhere)
	bool bEncryptAllAssetFiles = false;
	// -encryptindex
	UPROPERTY(EditAnywhere)
    bool bEncryptIndex = false;
	// Use DefaultCrypto.ini
	UPROPERTY(EditAnywhere)
	bool bUseDefaultCryptoIni = false;
	// sign pak
	UPROPERTY(EditAnywhere,meta=(EditCondition="bUseDefaultCryptoIni"))
	bool bSign = false;
	// crypto.json (option)
	UPROPERTY(EditAnywhere,meta=(EditCondition="!bUseDefaultCryptoIni"))
	FFilePath CryptoKeys;
};

UENUM(BlueprintType)
enum class EShaderLibNameRule : uint8
{
	VERSION_ID,
	PROJECT_NAME,
	CUSTOM
};

USTRUCT(BlueprintType)
struct FCookShaderOptions
{
	GENERATED_BODY()
	FCookShaderOptions()
	{
		ShderLibMountPoint = FString::Printf(TEXT("../../../%s/ShaderLib"),FApp::GetProjectName());
	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSharedShaderLibrary = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNativeShader = false;
	// if name is StartContent to ShaderArchive-StarterContent-PCD3D_SM5.ushaderbytecode
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EShaderLibNameRule ShaderNameRule;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(EditCondition="ShaderNameRule==EShaderLibNameRule::CUSTOM"))
	FString CustomShaderName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ShderLibMountPoint;
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

	TArray<FExternFileInfo> GetAllExternFiles(bool InGeneratedHash=false)const;

	// override
	FORCEINLINE virtual TArray<FDirectoryPath>& GetAssetIncludeFilters() override{ return AssetIncludeFilters; }
	FORCEINLINE virtual TArray<FPatcherSpecifyAsset>& GetIncludeSpecifyAssets()override { return IncludeSpecifyAssets; }
	FORCEINLINE virtual TArray<FDirectoryPath>& GetAssetIgnoreFilters() override { return AssetIgnoreFilters; }
	FORCEINLINE virtual TArray<FPlatformExternAssets>& GetAddExternAssetsToPlatform()override{return AddExternAssetsToPlatform;}
	
	TArray<FString> GetAssetIgnoreFiltersPaths()const;
	FORCEINLINE bool IsAnalysisFilterDependencies()const { return bAnalysisFilterDependencies; }
	FORCEINLINE bool IsAnalysisDiffAssetDependenciesOnly()const {return bAnalysisDiffAssetDependenciesOnly;}
	FORCEINLINE bool IsRecursiveWidgetTree()const {return bRecursiveWidgetTree;}
	FORCEINLINE TArray<EAssetRegistryDependencyTypeEx> GetAssetRegistryDependencyTypes()const { return AssetRegistryDependencyTypes; }
	
	FORCEINLINE FString GetVersionId()const { return VersionId; }
	FString GetBaseVersion()const;
	FORCEINLINE TArray<FString> GetUnrealPakListOptions()const { return GetUnrealPakSettings().UnrealPakListOptions; }
	FORCEINLINE TArray<FReplaceText> GetReplacePakListTexts()const { return ReplacePakListTexts; }
	FORCEINLINE TArray<FString> GetUnrealPakCommandletOptions()const { return GetUnrealPakSettings().UnrealCommandletOptions; }
	FORCEINLINE TArray<ETargetPlatform> GetPakTargetPlatforms()const { return PakTargetPlatforms; }
	TArray<FString> GetPakTargetPlatformNames()const;

	// FORCEINLINE bool IsSavePakList()const { return bStoragePakList; }
	FORCEINLINE bool IsSaveDiffAnalysis()const { return IsByBaseVersion() && bStorageDiffAnalysisResults; }
	FORCEINLINE TArray<FString> GetIgnoreDeletionModulesAsset()const{return IgnoreDeletionModulesAsset;}

	FORCEINLINE bool IsForceSkipContent()const{return bForceSkipContent;}
	FORCEINLINE TArray<FDirectoryPath> GetForceSkipContentRules()const {return ForceSkipContentRules;}
	FORCEINLINE TArray<FSoftObjectPath> GetForceSkipAssets()const {return ForceSkipAssets;}
	TArray<FString> GetForceSkipContentStrRules()const;
	TArray<FString> GetForceSkipAssetsStr()const;
	
	FORCEINLINE bool IsIncludeAssetRegistry()const { return bIncludeAssetRegistry; }
	FORCEINLINE bool IsIncludeGlobalShaderCache()const { return bIncludeGlobalShaderCache; }
	FORCEINLINE bool IsIncludeShaderBytecode()const { return bIncludeShaderBytecode; }
	FORCEINLINE bool IsMakeBinaryConfig()const { return bMakeBinaryConfig; }
	FORCEINLINE bool IsIncludeEngineIni()const { return bIncludeEngineIni; }
	FORCEINLINE bool IsIncludePluginIni()const { return bIncludePluginIni; }
	FORCEINLINE bool IsIncludeProjectIni()const { return bIncludeProjectIni; }

	FORCEINLINE bool IsByBaseVersion()const { return bByBaseVersion; }
	FORCEINLINE bool IsEnableExternFilesDiff()const { return bEnableExternFilesDiff; }
	FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return bIncludeHasRefAssetsOnly; }
	FORCEINLINE bool IsIncludePakVersion()const { return bIncludePakVersionFile; }
	FORCEINLINE bool IsSaveAssetRelatedInfo()const { return bStorageAssetDependencies; }

	// chunk infomation
	FORCEINLINE bool IsEnableChunk()const { return bEnableChunk; }
	FORCEINLINE TArray<FChunkInfo> GetChunkInfos()const { return ChunkInfos; }

	FORCEINLINE FString GetPakVersionFileMountPoint()const { return PakVersionFileMountPoint; }
	FORCEINLINE TArray<FExternFileInfo> GetAddExternFiles()const { return AddExternFileToPak; }
	FORCEINLINE TArray<FExternDirectoryInfo> GetAddExternDirectory()const { return AddExternDirectoryToPak; }
	static FPakVersion GetPakVersion(const FHotPatcherVersion& InHotPatcherVersion,const FString& InUtcTime);
	static FString GetSavePakVersionPath(const FString& InSaveAbsPath,const FHotPatcherVersion& InVersion);
	static FString GetPakCommandsSaveToPath(const FString& InSaveAbsPath, const FString& InPlatfornName, const FHotPatcherVersion& InVersion);

	FHotPatcherVersion GetNewPatchVersionInfo();
	bool GetBaseVersionInfo(FHotPatcherVersion& OutBaseVersion)const;
	FString GetCurrentVersionSavePath()const;

	FORCEINLINE bool IsCustomPakNameRegular()const {return bCustomPakNameRegular;}
	FORCEINLINE FString GetPakNameRegular()const { return PakNameRegular;}
	FORCEINLINE bool IsCookPatchAssets()const {return bCookPatchAssets;}
	FORCEINLINE bool IsIgnoreDeleatedAssetsInfo()const {return bIgnoreDeleatedAssetsInfo;}
	FORCEINLINE bool IsSaveDeletedAssetsToNewReleaseJson()const {return bStorageDeletedAssetsToNewReleaseJson;}
	
	TArray<FString> GetAssetIncludeFiltersPaths()const;
	
	FORCEINLINE FIoStoreSettings GetIoStoreSettings()const { return IoStoreSettings; }
	FORCEINLINE FUnrealPakSettings GetUnrealPakSettings()const {return UnrealPakSettings;}
	FORCEINLINE TArray<FString> GetDefaultPakListOptions()const {return DefaultPakListOptions;}
	FORCEINLINE TArray<FString> GetDefaultCommandletOptions()const {return DefaultCommandletOptions;}
#if WITH_PACKAGE_CONTEXT
	virtual void InitPlatformPackageContexts();
	FORCEINLINE TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContexts()const {return PlatformSavePackageContexts;}
	bool SavePlatformBulkDataManifest(ETargetPlatform Platform);
#endif
	FORCEINLINE bool IsCreateDefaultChunk()const { return bCreateDefaultChunk; }
	FORCEINLINE bool IsEnableMultiThread()const{ return bEnableMultiThread; }

	FORCEINLINE bool IsStorageNewRelease()const{return bStorageNewRelease;}
	FORCEINLINE bool IsStoragePakFileInfo()const{return bStoragePakFileInfo;}
	
	FORCEINLINE FPakEncryptSettings GetEncryptSettings()const{ return EncryptSettings; }
	FORCEINLINE bool IsBinariesPatch()const{ return bBinariesPatch; }
	FORCEINLINE FBinariesPatchConfig GetBinariesPatchConfig()const{ return BinariesPatchConfig; }
	FORCEINLINE bool IsSharedShaderLibrary()const { return GetCookShaderOptions().bSharedShaderLibrary; }
	FORCEINLINE FCookShaderOptions GetCookShaderOptions()const {return CookShaderOptions;}
	FString GetShaderLibraryName()const;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion")
		bool bByBaseVersion = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "BaseVersion",meta = (RelativeToGameContentDir, EditCondition="bByBaseVersion"))
		FFilePath BaseVersion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchBaseSettings")
		FString VersionId;

	// require HDiffPatchUE plugin
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch")
		bool bBinariesPatch;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BinariesPatch", meta=(EditCondition="bBinariesPatch"))
		FBinariesPatchConfig BinariesPatchConfig;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Asset Filter",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	// Ignore directories in AssetIncludeFilters 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter")
		bool bForceSkipContent = true;
	// force exclude asset folder e.g. Exclude editor content when cooking in Project Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Asset Filter",meta = (RelativeToGameContentDir, LongPackageName, EditCondition="bForceSkipContent"))
    	TArray<FDirectoryPath> ForceSkipContentRules;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Asset Filter",meta = (EditCondition="bForceSkipContent"))
		TArray<FSoftObjectPath> ForceSkipAssets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter")
		bool bIncludeHasRefAssetsOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter",meta = (EditCondition = "!bAnalysisDiffAssetDependenciesOnly"))
		bool bAnalysisFilterDependencies=true;
	// 只对与基础包有差异的资源进行依赖分析，提高依赖分析的速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter",meta = (EditCondition = "!bAnalysisFilterDependencies"))
		bool bAnalysisDiffAssetDependenciesOnly=false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter")
		bool bRecursiveWidgetTree = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter",meta = (EditCondition="bAnalysisFilterDependencies || bAnalysisDiffAssetDependenciesOnly"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Specify Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeAssetRegistry = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeGlobalShaderCache = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern Files")
		bool bEnableExternFilesDiff = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern Files")
		TArray<FString> IgnoreDeletionModulesAsset;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern Files")
		TArray<FExternFileInfo> AddExternFileToPak;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern Files")
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern Files")
		TArray<FPlatformExternAssets> AddExternAssetsToPlatform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern Files")
		bool bIncludePakVersionFile = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extern Files",meta=(EditCondition = "bIncludePakVersionFile"))
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
		bool bCookPatchAssets = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options", meta=(EditCondition = "bCookPatchAssets"))
		FCookShaderOptions CookShaderOptions;
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
	// // crypto.json
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
	// 	FFilePath CryptoKeys;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FReplaceText> ReplacePakListTexts;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<ETargetPlatform> PakTargetPlatforms;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bCustomPakNameRegular = false;
	// Can use value: {VERSION} {BASEVERSION} {CHUNKNAME} {PLATFORM} 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options",meta=(EditCondition = "bCustomPakNameRegular"))
		FString PakNameRegular = TEXT("{VERSION}_{CHUNKNAME}_{PLATFORM}_001_P");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStorageNewRelease = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStoragePakFileInfo = true;
	// dont display deleted asset info in patcher
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bIgnoreDeleatedAssetsInfo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStorageDeletedAssetsToNewReleaseJson = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo",meta=(EditCondition="bByBaseVersion"))
		bool bStorageDiffAnalysisResults = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bStorageAssetDependencies = false;


	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
		bool bEnableMultiThread = false;
private:
	TMap<ETargetPlatform,class FSavePackageContext*> PlatformSavePackageContexts;
};