// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
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

// engine header
#include "CoreMinimal.h"
#include "FPlatformExternAssets.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "FExportPatchSettings.generated.h"


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
	FORCEINLINE bool IsRecursiveWidgetTree()const {return bRecursiveWidgetTree;}
	FORCEINLINE TArray<EAssetRegistryDependencyTypeEx> GetAssetRegistryDependencyTypes()const { return AssetRegistryDependencyTypes; }
	

	// pak command
	TArray<FString> MakeAddExternFileToPakCommands()const;
	TArray<FString> MakeAllExternDirectoryAsPakCommand()const;
	TArray<FString> MakeAllPakCommandsByTheSetting(const FString& InPlatformName, const FPatchVersionDiff& InVersionDiff, bool bDiffExFiles = true)const;
	bool MakeAllExternAssetAsPakCommands(const FString& InProjectDir, const FString& InPlatform, const TArray<FString>& PakOptions, TArray<FString>& OutPakCommands)const;

	FORCEINLINE FString GetVersionId()const { return VersionId; }
	FString GetBaseVersion()const;
	FORCEINLINE TArray<FString> GetPakCommandOptions()const { return PakCommandOptions; }
	FORCEINLINE TArray<FReplaceText> GetReplacePakCommandTexts()const { return ReplacePakCommandTexts; }
	FORCEINLINE TArray<FString> GetUnrealPakOptions()const { return UnrealPakOptions; }
	FORCEINLINE TArray<ETargetPlatform> GetPakTargetPlatforms()const { return PakTargetPlatforms; }
	TArray<FString> GetPakTargetPlatformNames()const;

	FORCEINLINE bool IsSavePakList()const { return bSavePakList; }
	FORCEINLINE bool IsSaveDiffAnalysis()const { return IsByBaseVersion() && bSaveDiffAnalysis; }
	FORCEINLINE TArray<FString> GetIgnoreDeletionModulesAsset()const{return IgnoreDeletionModulesAsset;}

	FORCEINLINE bool IsForceSkipContent()const{return bForceSkipContent;}
	FORCEINLINE TArray<FDirectoryPath> GetForceSkipContentRules()const {return ForceSkipContentRules;}
	FORCEINLINE TArray<FSoftObjectPath> GetForceSkipAssets()const {return ForceSkipAssets;}
	TArray<FString> GetForceSkipContentStrRules()const;
	TArray<FString> GetForceSkipAssetsStr()const;
	
	FORCEINLINE bool IsIncludeAssetRegistry()const { return bIncludeAssetRegistry; }
	FORCEINLINE bool IsIncludeGlobalShaderCache()const { return bIncludeGlobalShaderCache; }
	FORCEINLINE bool IsIncludeShaderBytecode()const { return bIncludeShaderBytecode; }
	FORCEINLINE bool IsIncludeEngineIni()const { return bIncludeEngineIni; }
	FORCEINLINE bool IsIncludePluginIni()const { return bIncludePluginIni; }
	FORCEINLINE bool IsIncludeProjectIni()const { return bIncludeProjectIni; }

	FORCEINLINE bool IsByBaseVersion()const { return bByBaseVersion; }
	FORCEINLINE bool IsEnableExternFilesDiff()const { return bEnableExternFilesDiff; }
	FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return bIncludeHasRefAssetsOnly; }
	FORCEINLINE bool IsIncludePakVersion()const { return bIncludePakVersionFile; }
	FORCEINLINE bool IsSaveAssetRelatedInfo()const { return bSaveAssetRelatedInfo; }

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
	FORCEINLINE bool IsSaveDeletedAssetsToNewReleaseJson()const {return bSaveDeletedAssetsToNewReleaseJson;}
	
	TArray<FString> GetAssetIncludeFiltersPaths()const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BaseVersion")
		bool bByBaseVersion = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "BaseVersion",meta = (RelativeToGameContentDir, EditCondition="bByBaseVersion"))
		FFilePath BaseVersion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchBaseSettings")
		FString VersionId;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter")
		bool bAnalysisFilterDependencies=true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter")
		bool bRecursiveWidgetTree = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filter",meta = (EditCondition="bAnalysisFilterDependencies"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Specify Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeAssetRegistry = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeGlobalShaderCache = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooked Files")
		bool bIncludeShaderBytecode = false;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk Options", meta = (EditCondition = "bEnableChunk"))
		TArray<FChunkInfo> ChunkInfos;
	/*
	 * Cook Asset in current patch
	 * shader code gets saved inline inside material assets
	 * bShareMaterialShaderCode as false
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bCookPatchAssets = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FString> PakCommandOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FReplaceText> ReplacePakCommandTexts;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<FString> UnrealPakOptions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		TArray<ETargetPlatform> PakTargetPlatforms;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options")
		bool bCustomPakNameRegular = false;
	// Can use value: {VERSION} {BASEVERSION} {CHUNKNAME} {PLATFORM} 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak Options",meta=(EditCondition = "bCustomPakNameRegular"))
		FString PakNameRegular = TEXT("{VERSION}_{CHUNKNAME}_{PLATFORM}_001_P");
	// dont display deleted asset info in patcher
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bIgnoreDeleatedAssetsInfo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveDeletedAssetsToNewReleaseJson = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSavePakList = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo",meta=(EditCondition="bByBaseVersion"))
		bool bSaveDiffAnalysis = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveAssetRelatedInfo = false;
};