// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
#include "FPatcherSpecifyAsset.h"

// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "ExportReleaseSettings.generated.h"

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
UCLASS()
class UExportReleaseSettings : public UObject
{
	GENERATED_BODY()
public:
	UExportReleaseSettings()
	{
		AssetRegistryDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);
	}

	FORCEINLINE static UExportReleaseSettings* Get()
	{
		static bool bInitialized = false;
		// This is a singleton, use default object
		UExportReleaseSettings* DefaultSettings = GetMutableDefault<UExportReleaseSettings>();
		DefaultSettings->AddToRoot();
		if (!bInitialized)
		{
			bInitialized = true;
		}

		return DefaultSettings;
	}

	FORCEINLINE bool SerializeReleaseConfigToString(FString& OutJsonString)
	{
		TSharedPtr<FJsonObject> ReleaseConfigJsonObject = MakeShareable(new FJsonObject);
		SerializeReleaseConfigToJsonObject(ReleaseConfigJsonObject);

		auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutJsonString);
		return FJsonSerializer::Serialize(ReleaseConfigJsonObject.ToSharedRef(), JsonWriter);
	}

	FORCEINLINE bool SerializeReleaseConfigToJsonObject(TSharedPtr<FJsonObject>& OutJsonObject)
	{
		return UFlibHotPatcherEditorHelper::SerializeReleaseConfigToJsonObject(this, OutJsonObject);
	}

	FORCEINLINE FString GetVersionId()const
	{
		return VersionId;
	}
	FORCEINLINE TArray<FString> GetAssetIncludeFiltersPaths()const
	{
		TArray<FString> Result;
		for (const auto& Filter : AssetIncludeFilters)
		{
			if (!Filter.Path.IsEmpty())
			{
				Result.AddUnique(Filter.Path);
			}
		}
		return Result;
	}
	FORCEINLINE TArray<FString> GetAssetIgnoreFiltersPaths()const
	{
		TArray<FString> Result;
		for (const auto& Filter : AssetIgnoreFilters)
		{
			if (!Filter.Path.IsEmpty())
			{
				Result.AddUnique(Filter.Path);
			}
		}
		return Result;
	}

	FORCEINLINE TArray<FExternAssetFileInfo> GetAllExternFiles(bool InGeneratedHash = false)const
	{
		TArray<FExternAssetFileInfo> AllExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(GetAddExternDirectory());

		for (auto& ExFile : GetAddExternFiles())
		{
			if (!AllExternFiles.Contains(ExFile))
			{
				AllExternFiles.Add(ExFile);
			}
		}
		if (InGeneratedHash)
		{
			for (auto& ExFile : AllExternFiles)
			{
				ExFile.GenerateFileHash();
			}
		}
		return AllExternFiles;
	}
	FORCEINLINE TArray<EAssetRegistryDependencyTypeEx> GetAssetRegistryDependencyTypes()const { return AssetRegistryDependencyTypes; }
	FORCEINLINE FString GetSavePath()const{return SavePath.Path;}
	FORCEINLINE bool IsSaveConfig()const {return bSaveReleaseConfig;}
	FORCEINLINE bool IsSaveAssetRelatedInfo()const { return bSaveAssetRelatedInfo; }
	FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return bIncludeHasRefAssetsOnly; }

	FORCEINLINE TArray<FPatcherSpecifyAsset> GetSpecifyAssets()const { return IncludeSpecifyAssets; }

	FORCEINLINE TArray<FExternAssetFileInfo> GetAddExternFiles()const { return AddExternFileToPak; }
	FORCEINLINE TArray<FExternDirectoryInfo> GetAddExternDirectory()const { return AddExternDirectoryToPak; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Version")
		FString VersionId;

	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "ReleaseSetting|Asset Filters",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Asset Filters", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Asset Filters")
		bool bIncludeHasRefAssetsOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Asset Filters")
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Specify Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Extern Files")
		TArray<FExternAssetFileInfo> AddExternFileToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Extern Files")
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveAssetRelatedInfo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveReleaseConfig;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		FDirectoryPath SavePath;
};