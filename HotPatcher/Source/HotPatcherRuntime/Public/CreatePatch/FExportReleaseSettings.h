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
#include "CreatePatch/HotPatcherSettingBase.h"

// engine header
#include "Misc/FileHelper.h"
#include "CoreMinimal.h"
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
	TArray<FFilePath> PakResponseFiles;
	UPROPERTY(EditAnywhere)
	TArray<FFilePath> PakFiles;
	UPROPERTY(EditAnywhere)
	FString AESKey;
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
	
	static FExportReleaseSettings* Get();
	FString GetVersionId()const;
	
	TArray<FPlatformExternAssets> GetAddExternAssetsToPlatform()const{return AddExternAssetsToPlatform;}

	FORCEINLINE bool IsBackupMetadata()const {return bBackupMetadata;}
	FORCEINLINE bool IsBackupProjectConfig()const {return bBackupProjectConfig;}
	FORCEINLINE bool IsByPakList()const { return ByPakList; }
	FORCEINLINE TArray<FPlatformPakListFiles> GetPlatformsPakListFiles()const {return PlatformsPakListFiles;}
	FORCEINLINE TArray<ETargetPlatform> GetBackupMetadataPlatforms()const{return BackupMetadataPlatforms;}
	
	virtual TArray<FPlatformExternAssets>& GetAddExternAssetsToPlatform()override{ return AddExternAssetsToPlatform;}
	FORCEINLINE bool IsImportProjectSettings()const{return bImportProjectSettings;}
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Version")
		FString VersionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version")
		bool ByPakList = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version", meta = (RelativeToGameContentDir, EditCondition = "ByPakList"))
		TArray<FPlatformPakListFiles> PlatformsPakListFiles;

	 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
		bool bImportProjectSettings = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "External Files")
		TArray<FPlatformExternAssets> AddExternAssetsToPlatform;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "SaveTo")
		bool bBackupMetadata = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "SaveTo")
		bool bBackupProjectConfig = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo",meta=(EditCondition="bBackupMetadata"))
		TArray<ETargetPlatform> BackupMetadataPlatforms;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Advanced")
		bool bNoShaderCompile = true;
};