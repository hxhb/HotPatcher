// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
#include "FPatcherSpecifyAsset.h"
#include "FExternFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPlatformExternAssets.h"
#include "HotPatcherLog.h"
#include "CreatePatch/HotPatcherSettingBase.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
// engine header
#include "Misc/FileHelper.h"
#include "CoreMinimal.h"



#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetStringLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "FExportReleaseSettings.generated.h"


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPlatformPakListFiles
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	ETargetPlatform TargetPlatform = ETargetPlatform::None;
	UPROPERTY(EditAnywhere)
	TArray<FFilePath> PakLists;
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPlatformPakAssets
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere)
		ETargetPlatform Platform;
	UPROPERTY(EditAnywhere)
		TArray<FPatcherSpecifyAsset> Assets;
	UPROPERTY(EditAnywhere)
		TArray<FExternFileInfo> ExternFiles;
};

/** Singleton wrapper to allow for using the setting structur e in SSettingsView */
USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FExportReleaseSettings:public FHotPatcherSettingBase
{
	GENERATED_USTRUCT_BODY()
public:
	FExportReleaseSettings();
	~FExportReleaseSettings();
	virtual void Init() override;
	virtual void ImportPakLists();
	virtual void ClearImportedPakList();
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	virtual void PostEditChangeProperty(const FPropertyChangedEvent& PropertyChangedEvent);
	virtual bool ParseByPaklist(FExportReleaseSettings* InReleaseSetting,const TArray<FString>& InPaklistFile);
	virtual FPlatformPakAssets PlatformPakListParser(const ETargetPlatform Platform, const TArray<FString>& );
	static FExportReleaseSettings* Get();
	FString GetVersionId()const;
	TArray<FString> GetAssetIncludeFiltersPaths()const;
	TArray<FString> GetAssetIgnoreFiltersPaths()const;
	TArray<FExternFileInfo> GetAllExternFiles(bool InGeneratedHash=false)const;
	TArray<FExternFileInfo> GetAllExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash = false)const;
	TMap<ETargetPlatform,FPlatformExternFiles> GetAllPlatfotmExternFiles(bool InGeneratedHash = false)const;
	TArray<FExternFileInfo> GetAddExternFilesByPlatform(ETargetPlatform InTargetPlatform)const;
	TArray<FExternDirectoryInfo> GetAddExternDirectoryByPlatform(ETargetPlatform InTargetPlatform)const;

	TArray<FPlatformExternAssets> GetAddExternAssetsToPlatform()const{return AddExternAssetsToPlatform;}
	FORCEINLINE FString GetSavePath()const{return SavePath.Path;}
	FORCEINLINE bool IsSaveConfig()const {return bSaveReleaseConfig;}
	FORCEINLINE bool IsSaveAssetRelatedInfo()const { return bSaveAssetRelatedInfo; }
	FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return bIncludeHasRefAssetsOnly; }
	FORCEINLINE bool IsAnalysisFilterDependencies()const { return bAnalysisFilterDependencies; }
	FORCEINLINE TArray<EAssetRegistryDependencyTypeEx> GetAssetRegistryDependencyTypes()const { return AssetRegistryDependencyTypes; }
	FORCEINLINE TArray<FPatcherSpecifyAsset> GetSpecifyAssets()const { return IncludeSpecifyAssets; }
	FORCEINLINE bool AddSpecifyAsset(FPatcherSpecifyAsset const& InAsset)
	{
		IncludeSpecifyAssets.AddUnique(InAsset);
		return true;
	}

	FORCEINLINE TArray<FExternFileInfo> GetAddExternFiles()const { return AddExternFileToPak; }
	FORCEINLINE TArray<FExternDirectoryInfo> GetAddExternDirectory()const { return AddExternDirectoryToPak; }

	FORCEINLINE bool IsByPakList()const { return ByPakList; }
	FORCEINLINE TArray<FPlatformPakListFiles> GetPlatformsPakListFiles()const {return PlatformsPakListFiles;}
	
	// override
	virtual TArray<FDirectoryPath>& GetAssetIncludeFilters() override
	{
		return AssetIncludeFilters;
	}
	virtual TArray<FDirectoryPath>& GetAssetIgnoreFilters()override
	{
		return AssetIgnoreFilters;
	}
	virtual TArray<FPatcherSpecifyAsset>& GetIncludeSpecifyAssets()override
	{
		return IncludeSpecifyAssets;
	}
	virtual TArray<FPlatformExternAssets>& GetAddExternAssetsToPlatform()override
	{
		return AddExternAssetsToPlatform;
	}
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Version")
		FString VersionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version")
		bool ByPakList = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version", meta = (RelativeToGameContentDir, EditCondition = "ByPakList"))
		TArray<FPlatformPakListFiles> PlatformsPakListFiles;
	
	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "AssetFilters",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters")
		bool bAnalysisFilterDependencies = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters",meta=(EditCondition="bAnalysisFilterDependencies"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters")
		bool bIncludeHasRefAssetsOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecifyAssets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExternFiles")
		TArray<FExternFileInfo> AddExternFileToPak;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExternFiles")
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExternFiles")
		TArray<FPlatformExternAssets> AddExternAssetsToPlatform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveAssetRelatedInfo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveReleaseConfig = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		FDirectoryPath SavePath;
};