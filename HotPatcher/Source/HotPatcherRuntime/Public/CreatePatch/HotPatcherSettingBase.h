#pragma once
#include "FPatcherSpecifyAsset.h"
#include "FPlatformExternAssets.h"
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
    virtual void Init(){};
    virtual ~FHotPatcherSettingBase(){}
};