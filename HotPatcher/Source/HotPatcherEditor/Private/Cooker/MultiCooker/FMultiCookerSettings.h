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
struct HOTPATCHEREDITOR_API FMultiCookerSettings: public FHotPatcherCookerSettingBase
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Filter",meta = (RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> IncludeFilters;
	// Ignore directories in AssetIncludeFilters 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter", meta = (RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> IgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bForceSkipContent = true;
	// force exclude asset folder e.g. Exclude editor content when cooking in Project Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Filter",meta = (RelativeToGameContentDir, LongPackageName, EditCondition="bForceSkipContent"))
	TArray<FDirectoryPath> ForceSkipFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Filter",meta = (EditCondition="bForceSkipContent"))
	TArray<FSoftObjectPath> ForceSkipAssets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bIncludeHasRefAssetsOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter",meta = (EditCondition = "!bAnalysisDiffAssetDependenciesOnly"))
	bool bAnalysisFilterDependencies=true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	bool bRecursiveWidgetTree = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter",meta = (EditCondition="bAnalysisFilterDependencies || bAnalysisDiffAssetDependenciesOnly"))
	TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	TArray<ETargetPlatform> CookTargetPlatforms;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker")
	FCookerShaderOptions ShaderOptions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooker",meta=(ClampMin=1,ClampMax=20))
	int32 ProcessNumber = 1;
};
