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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
	bool bSharedShaderLibrary = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
	bool bNativeShader = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bSharedShaderLibrary"))
	bool bMergeShaderLibrary = false;
};

USTRUCT(BlueprintType)
struct HOTPATCHERCORE_API FSingleCookerSettings:public FHotPatcherCookerSettingBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	FString MissionName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	int32 MissionID = -1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	FString ShaderLibName;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	TArray<FAssetDetail> CookAssets;

	// skip loaded assets when cooking(package path)
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	TSet<FName> SkipLoadedAssets;
	
	// Directories or asset package path
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	TArray<FString> SkipCookContents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="")
	TArray<UClass*> ForceSkipClasses;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	TArray<ETargetPlatform> CookTargetPlatforms;

	// track load asset when cooking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bPackageTracker = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	FCookerShaderOptions ShaderOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bSerializeAssetRegistry = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	// FAssetRegistryOptions SerializeAssetRegistryOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	FIoStoreSettings IoStoreSettings;

	// cook load in cooking assets by SingleCookder side
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bCookAdditionalAssets = true;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	bool bAsyncLoad = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="!bAsyncLoad"))
	bool bPreGeneratePlatformData = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bPreGeneratePlatformData"))
	bool bWaitEachAssetCompleted = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(EditCondition="bPreGeneratePlatformData"))
	bool bConcurrentSave = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDisplayConfig = false;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	FString StorageCookedDir;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	FString StorageMetadataDir;

	// UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SavePackageContext")
#if WITH_PACKAGE_CONTEXT
	bool bOverrideSavePackageContext = false;
	TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
#endif
	bool IsSkipAsset(const FString& PackageName);
};


USTRUCT(BlueprintType)
struct HOTPATCHERCORE_API FAssetsCollection
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	ETargetPlatform TargetPlatform = ETargetPlatform::None;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	TArray<FAssetDetail> Assets;
};

USTRUCT(BlueprintType)
struct HOTPATCHERCORE_API FCookerFailedCollection
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	FString MissionName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	int32 MissionID = -1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
	TMap<ETargetPlatform,FAssetsCollection> CookFailedAssets;
};


FORCEINLINE bool FSingleCookerSettings::IsSkipAsset(const FString& PackageName)
{
	bool bRet = false;
	for(const auto& SkipCookContent:SkipCookContents)
	{
		if(PackageName.StartsWith(SkipCookContent))
		{
			bRet = true;
			break;
		}
	}
	return bRet;
}
