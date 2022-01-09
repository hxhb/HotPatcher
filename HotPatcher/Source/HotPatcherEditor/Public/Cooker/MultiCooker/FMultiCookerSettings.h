// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
#include "HotPatcherLog.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
#include "Cooker/HotPatcherCookerSettingBase.h"

// engine header
#include "Misc/FileHelper.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetStringLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "FMultiCookerSettings.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHEREDITOR_API FCookerShaderOptions
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSharedShaderLibrary = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNativeShader = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bSharedShaderLibrary"))
	bool bMergeShaderLibrary = false;
};


/** Singleton wrapper to allow for using the setting structur e in SSettingsView */
USTRUCT(BlueprintType)
struct HOTPATCHEREDITOR_API FMultiCookerSettings: public FHotPatcherSettingBase
{
	GENERATED_USTRUCT_BODY()
public:
	FMultiCookerSettings();

	static FMultiCookerSettings* Get();

	bool IsValidConfig()const;
	bool IsSkipAssets(FName LongPacakgeName);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset Filters")
	bool bImportProjectSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	TSubclassOf<class UMultiCookScheduler> Scheduler;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	TArray<ETargetPlatform> CookTargetPlatforms;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	FCookerShaderOptions ShaderOptions;
	// Compile Global Shader on Multi Cooker
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bCompileGlobalShader = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bSerializeAssetRegistry;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	// FAssetRegistryOptions SerializeAssetRegistryOptions;
	//
	// track load asset when cooking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bPackageTracker = true;
	// support UE4.26 later
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	FIoStoreSettings IoStoreSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(ClampMin=1,ClampMax=20))
	int32 ProcessNumber = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profiling")
	bool bProfilingMultiCooker = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profiling")
	bool bProfilingPerSingleCooker = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profiling")
	bool bDisplayMissionConfig = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bSkipCook = false;
};


USTRUCT(BlueprintType)
struct HOTPATCHEREDITOR_API FSingleCookerSettings:public FHotPatcherCookerSettingBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString MissionName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 MissionID = -1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString ShaderLibName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FAssetDetail> CookAssets;
	// UPROPERTY()
	// TArray<FAssetDetail> FaildAssets;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FMultiCookerSettings MultiCookerSettings;
};


USTRUCT(BlueprintType)
struct HOTPATCHEREDITOR_API FAssetsCollection
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	ETargetPlatform TargetPlatform = ETargetPlatform::None;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FAssetDetail> Assets;
};

USTRUCT(BlueprintType)
struct HOTPATCHEREDITOR_API FCookerFailedCollection
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString MissionName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 MissionID = -1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TMap<ETargetPlatform,FAssetsCollection> CookFailedAssets;
};


USTRUCT()
struct HOTPATCHEREDITOR_API FMultiCookerAssets
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FAssetDetail> Assets;
};