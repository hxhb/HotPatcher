#pragma once
#include "FPatcherSpecifyAsset.h"
#include "FPlatformExternAssets.h"
#include "Struct/AssetManager/FAssetDependenciesInfo.h"

// engine 
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "HotPatcherSettingBase.generated.h"



USTRUCT()
struct HOTPATCHERRUNTIME_API FHotPatcherSettingBase
{
    GENERATED_USTRUCT_BODY()

    virtual TArray<FDirectoryPath>& GetAssetIncludeFilters()
    {
        static TArray<FDirectoryPath> TempDir;
        return TempDir;
    };
    virtual TArray<FDirectoryPath>& GetAssetIgnoreFilters()
    {
        static TArray<FDirectoryPath> TempDir;
        return TempDir;
    }
    virtual TArray<FPatcherSpecifyAsset>& GetIncludeSpecifyAssets()
    {
         static TArray<FPatcherSpecifyAsset> TempAssets;
        return TempAssets;
    };
    virtual TArray<FPlatformExternAssets>& GetAddExternAssetsToPlatform()
    {
        static TArray<FPlatformExternAssets> PlatformNoAssets;
        return PlatformNoAssets;
    };
    virtual TMap<FString,FAssetDependenciesInfo>& GetAssetsDependenciesScanedCaches()
    {
        return ScanedCaches;
    }
    virtual bool IsScanCacheOptimize()const{return bScanCacheOptimize;}
    virtual void Init(){};
    virtual ~FHotPatcherSettingBase(){}
protected:
    // UPROPERTY(EditAnywhere, Category = "Advanced")
    bool bScanCacheOptimize=true;
    TMap<FString,FAssetDependenciesInfo> ScanedCaches;
};