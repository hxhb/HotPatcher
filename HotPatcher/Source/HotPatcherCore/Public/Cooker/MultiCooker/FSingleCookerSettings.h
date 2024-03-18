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
#include "FSingleCookerSettings.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHERCORE_API FCookerShaderOptions
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSharedShaderLibrary = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNativeShader = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bSharedShaderLibrary"))
	bool bMergeShaderLibrary = false;
};

USTRUCT(BlueprintType)
struct HOTPATCHERCORE_API FSingleCookerSettings:public FHotPatcherCookerSettingBase
{
	GENERATED_BODY()
public:
	FSingleCookerSettings();
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString MissionName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 MissionID = -1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool bShaderCooker = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString ShaderLibName;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FAssetDetail> CookAssets;

	// skip loaded assets when cooking(package path)
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSet<FName> SkipLoadedAssets;
	
	// Directories or asset package path
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FString> SkipCookContents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UClass*> ForceSkipClasses;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	TArray<ETargetPlatform> CookTargetPlatforms;

	// track load asset when cooking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bPackageTracker = true;
	// cook load in cooking assets by SingleCookder side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bPackageTracker"))
	bool bCookPackageTrackerAssets = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	FCookerShaderOptions ShaderOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bSerializeAssetRegistry = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	// FAssetRegistryOptions SerializeAssetRegistryOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	FIoStoreSettings IoStoreSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bForceCookInOneFrame = false;
	FORCEINLINE int32 GetNumberOfAssetsPerFrame()const{ return NumberOfAssetsPerFrame; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="!bForceCookInOneFrame",ClampMin=0))
	int32 NumberOfAssetsPerFrame = 100;
	TMap<UClass*,int32> OverrideNumberOfAssetsPerFrame;
	FORCEINLINE const TMap<UClass*,int32>& GetOverrideNumberOfAssetsPerFrame()const{ return OverrideNumberOfAssetsPerFrame; }
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bAsyncLoad = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="!bAsyncLoad"))
	bool bPreGeneratePlatformData = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bPreGeneratePlatformData"))
	bool bWaitEachAssetCompleted = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="!bAsyncLoad"))
	bool bCachePlatformDataOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bPreGeneratePlatformData && !bCachePlatformDataOnly"))
	bool bConcurrentSave = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bAllowRegisteAdditionalWorker = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bAccompanyCook = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDisplayConfig = false;

	FString GetStorageCookedAbsDir()const;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString StorageCookedDir;

	FString GetStorageMetadataAbsDir()const;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString StorageMetadataDir;

	// UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SavePackageContext")
#if WITH_PACKAGE_CONTEXT
	bool bOverrideSavePackageContext = false;
	TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
#endif
	bool IsSkipAsset(const FString& PackageName);
};

USTRUCT(BlueprintType)
struct FPackagePathSet
{
	GENERATED_BODY()

	UPROPERTY()
	TSet<FName> PackagePaths;
};
//
// USTRUCT(BlueprintType)
// struct HOTPATCHERCORE_API FAssetsCollection
// {
// 	GENERATED_BODY()
// 	UPROPERTY(EditAnywhere,BlueprintReadWrite)
// 	ETargetPlatform TargetPlatform = ETargetPlatform::None;
// 	UPROPERTY(EditAnywhere,BlueprintReadWrite)
// 	TArray<FAssetDetail> Assets;
// };

USTRUCT(BlueprintType)
struct HOTPATCHERCORE_API FCookerFailedCollection
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString MissionName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 MissionID = -1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TMap<ETargetPlatform,FPackagePathSet> CookFailedAssets;
};
