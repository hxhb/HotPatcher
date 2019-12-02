// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

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
	}

	FORCEINLINE static UExportReleaseSettings* Get()
	{
		static bool bInitialized = false;
		// This is a singleton, use default object
		UExportReleaseSettings* DefaultSettings = GetMutableDefault<UExportReleaseSettings>();

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
	FORCEINLINE TArray<FString> GetAssetIncludeFilters()const
	{
		TArray<FString> Result;
		for (const auto& Filter : AssetIncludeFilters)
		{
			Result.AddUnique(Filter.Path);
		}
		return Result;
	}
	FORCEINLINE TArray<FString> GetAssetIgnoreFilters()const
	{
		TArray<FString> Result;
		for (const auto& Filter : AssetIgnoreFilters)
		{
			Result.AddUnique(Filter.Path);
		}
		return Result;
	}
	FORCEINLINE FString GetSavePath()const
	{
		return SavePath.Path;
	}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Version")
		FString VersionId;

	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "AssetFilters",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		FDirectoryPath SavePath;
};