// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "ExportPatchSettings.generated.h"

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
UCLASS()
class UExportPatchSettings : public UObject
{
	GENERATED_BODY()
public:
	UExportPatchSettings(){}

	FORCEINLINE static UExportPatchSettings* Get()
	{
		static bool bInitialized = false;
		// This is a singleton, use default object
		UExportPatchSettings* DefaultSettings = GetMutableDefault<UExportPatchSettings>();

		if (!bInitialized)
		{
			bInitialized = true;
		}

		return DefaultSettings;
	}

	FORCEINLINE FString GetVersionId()const
	{
		return VersionId;
	}
	FORCEINLINE TArray<FString> GetAssetFilters()const
	{
		TArray<FString> Result;
		for (const auto& Filter : AssetFilters)
		{
			Result.AddUnique(Filter.Path);
		}
		return Result;
	}
	FORCEINLINE FString GetBaseVersion()const { return BaseVersion.FilePath; }
	FORCEINLINE FString GetSaveAbsPath()const {
		if (!SavePath.Path.IsEmpty())
		{
			return FPaths::ConvertRelativePathToFull(SavePath.Path);
		}
		return TEXT("");
	}
	FORCEINLINE bool IsSavePakList()const { return bSavePakList; }
	FORCEINLINE bool IsSaveDiffAnalysis()const { return bSaveDiffAnalysis; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "BaseVersion",meta = (RelativeToGameContentDir))
		FFilePath BaseVersion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchSettings")
		FString VersionId;

	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "PatchSettings|AssetFilter",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetFilters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		bool bSavePakList;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		bool bSaveDiffAnalysis;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		FDirectoryPath SavePath;

};