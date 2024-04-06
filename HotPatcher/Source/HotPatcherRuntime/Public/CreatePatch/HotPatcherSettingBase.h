#pragma once
#include "FPlatformExternFiles.h"
#include "FPatcherSpecifyAsset.h"
#include "FPlatformExternAssets.h"
#include "BaseTypes/AssetManager/FAssetDependenciesInfo.h"
// engine
#include "CoreMinimal.h"
#include "FAssetScanConfig.h"
#include "Engine/EngineTypes.h"
#include "HotPatcherSettingBase.generated.h"

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPatcherEntitySettingBase
{
    GENERATED_BODY();
    virtual ~FPatcherEntitySettingBase(){}
};


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FHotPatcherSettingBase:public FPatcherEntitySettingBase
{
    GENERATED_USTRUCT_BODY()
    FHotPatcherSettingBase();
    
    virtual TArray<FPlatformExternAssets>& GetAddExternAssetsToPlatform();
    virtual void Init();

    virtual TArray<FExternFileInfo> GetAllExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash = false);
    virtual TMap<ETargetPlatform,FPlatformExternFiles> GetAllPlatfotmExternFiles(bool InGeneratedHash = false);
    virtual TArray<FExternFileInfo> GetAddExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash);
    virtual TArray<FExternDirectoryInfo> GetAddExternDirectoryByPlatform(ETargetPlatform InTargetPlatform);

    virtual FString GetSaveAbsPath()const;
    FORCEINLINE_DEBUGGABLE virtual FString GetSavePath()const{ return SavePath.Path; }
    
    FORCEINLINE virtual bool IsStandaloneMode()const {return bStandaloneMode;}
    FORCEINLINE virtual bool IsSaveConfig()const {return bStorageConfig;}
    FORCEINLINE virtual TArray<FString> GetAdditionalCommandletArgs()const{return AdditionalCommandletArgs;}
    virtual FString GetCombinedAdditionalCommandletArgs()const;

    FORCEINLINE virtual bool IsForceSkipContent()const{return GetAssetScanConfig().bForceSkipContent;}
    FORCEINLINE virtual TArray<FDirectoryPath> GetForceSkipContentRules()const {return GetAssetScanConfig().ForceSkipContentRules;}
    FORCEINLINE virtual TArray<FSoftObjectPath> GetForceSkipAssets()const {return GetAssetScanConfig().ForceSkipAssets;}
    virtual TArray<FString> GetAllSkipContents()const;

    FORCEINLINE virtual TArray<FDirectoryPath>& GetAssetIncludeFilters() { return GetAssetScanConfigRef().AssetIncludeFilters; }
    FORCEINLINE virtual TArray<FPatcherSpecifyAsset>& GetIncludeSpecifyAssets() { return GetAssetScanConfigRef().IncludeSpecifyAssets; }
    FORCEINLINE virtual TArray<FDirectoryPath>& GetAssetIgnoreFilters()  { return GetAssetScanConfigRef().AssetIgnoreFilters; }
    FORCEINLINE TArray<FPatcherSpecifyAsset> GetSpecifyAssets()const { return GetAssetScanConfig().IncludeSpecifyAssets; }
    FORCEINLINE bool AddSpecifyAsset(FPatcherSpecifyAsset const& InAsset)
    {
        return GetAssetScanConfigRef().IncludeSpecifyAssets.AddUnique(InAsset) != INDEX_NONE;
    }
    FORCEINLINE virtual TArray<UClass*>& GetForceSkipClasses() { return GetAssetScanConfigRef().ForceSkipClasses; }
    // virtual TArray<FString> GetAssetIgnoreFiltersPaths()const;
    FORCEINLINE bool IsAnalysisFilterDependencies()const { return GetAssetScanConfig().bAnalysisFilterDependencies; }
    FORCEINLINE bool IsRecursiveWidgetTree()const {return GetAssetScanConfig().bRecursiveWidgetTree;}
    FORCEINLINE bool IsAnalysisMatInstance()const { return GetAssetScanConfig().bAnalysisMaterialInstance; }
    FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return GetAssetScanConfig().bIncludeHasRefAssetsOnly; }
    FORCEINLINE TArray<EAssetRegistryDependencyTypeEx> GetAssetRegistryDependencyTypes()const { return GetAssetScanConfig().AssetRegistryDependencyTypes; }
    FORCEINLINE bool IsPackageTracker()const { return GetAssetScanConfig().bPackageTracker; }

    FORCEINLINE FAssetScanConfig GetAssetScanConfig()const{ return AssetScanConfig; }
    FORCEINLINE FAssetScanConfig& GetAssetScanConfigRef() { return AssetScanConfig; }
    FORCEINLINE EHashCalculator GetHashCalculator()const { return HashCalculator; }
    virtual ~FHotPatcherSettingBase(){}
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Asset Filters")
    FAssetScanConfig AssetScanConfig;

    // backup current project Cooked/PLATFORM/PROJECTNAME/Metadata directory
    UPROPERTY(EditAnywhere, Category = "SaveTo")
    bool bStorageConfig = true;
    UPROPERTY(EditAnywhere, Category = "SaveTo")
    FDirectoryPath SavePath;
    
    UPROPERTY(EditAnywhere, Category = "Advanced")
    EHashCalculator HashCalculator = EHashCalculator::MD5;
    
    // create a UE4Editor-cmd.exe process execute patch mission.
    UPROPERTY(EditAnywhere, Category = "Advanced")
    bool bStandaloneMode = true;
    UPROPERTY(EditAnywhere, Category = "Advanced")
    TArray<FString> AdditionalCommandletArgs;
    
};