#pragma once
// project header
#include "FPatcherSpecifyAsset.h"
#include "FExternFileInfo.h"
#include "ETargetPlatform.h"
#include "FPlatformExternFiles.h"
#include "FPlatformExternAssets.h"

// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "FAssetScanConfig.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FAssetScanConfig
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPackageTracker = true;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite,meta = (RelativeToGameContentDir, LongPackageName))
    TArray<FDirectoryPath> AssetIncludeFilters;
    // Ignore directories in AssetIncludeFilters 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RelativeToGameContentDir, LongPackageName))
    TArray<FDirectoryPath> AssetIgnoreFilters;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIncludeHasRefAssetsOnly = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAnalysisFilterDependencies=true;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRecursiveWidgetTree = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAnalysisMaterialInstance = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSupportWorldComposition = false;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bForceSkipContent = true;
    // force exclude asset folder e.g. Exclude editor content when cooking in Project Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite,meta = (RelativeToGameContentDir, LongPackageName, EditCondition="bForceSkipContent"))
    TArray<FDirectoryPath> ForceSkipContentRules;
    UPROPERTY(EditAnywhere, BlueprintReadWrite,meta = (EditCondition="bForceSkipContent"))
    TArray<FSoftObjectPath> ForceSkipAssets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition="bForceSkipContent"))
    TArray<UClass*> ForceSkipClasses;

	bool IsMatchForceSkip(const FSoftObjectPath& ObjectPath,FString& OutReason);

};