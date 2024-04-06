#pragma once
#include "ETargetPlatform.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "FlibPatchParserHelper.h"
#include "CoreMinimal.h"
#include "Kismet/KismetTextLibrary.h"
#include "HotPatcherSettings.generated.h"
#define LOCTEXT_NAMESPACE "UHotPatcherSettings"

USTRUCT()
struct FPakExternalInfo
{
    GENERATED_BODY()
    FPakExternalInfo()=default;
    FPakExternalInfo(const FPakExternalInfo&)=default;
    UPROPERTY(EditAnywhere)
    FString PakName;
    UPROPERTY(EditAnywhere)
    TArray<ETargetPlatform> TargetPlatforms;
    UPROPERTY(EditAnywhere)
    FPlatformExternAssets AddExternAssetsToPlatform;
};

UCLASS(config = Game, defaultconfig)
class HOTPATCHERCORE_API UHotPatcherSettings:public UObject
{
    GENERATED_UCLASS_BODY()
public:
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    bool bWhiteListCookInEditor = false;
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    TArray<ETargetPlatform> PlatformWhitelists;

    FString GetTempSavedDir()const;
    FString GetHPLSavedDir()const;
    
    UPROPERTY(EditAnywhere, config, Category = "ConfigTemplate")
    FExportPatchSettings TempPatchSetting;
    
    UPROPERTY(EditAnywhere, config, Category = "Preset")
    TArray<FExportPatchSettings> PresetConfigs;

    UPROPERTY(EditAnywhere, config, Category = "Preview")
    bool bPreviewTooltips = true;
    UPROPERTY(EditAnywhere, config, Category = "Preview")
    bool bExternalFilesCheck = false;
    
    UPROPERTY(config)
    bool bServerlessCounter = true;
    UPROPERTY(EditAnywhere, config, Category = "Advanced")
    bool bServerlessCounterInCmdlet = false;
};


FORCEINLINE FString UHotPatcherSettings::GetTempSavedDir()const
{
    return UFlibPatchParserHelper::ReplaceMark(TempPatchSetting.SavePath.Path);
}
FORCEINLINE UHotPatcherSettings::UHotPatcherSettings(const FObjectInitializer& Initializer):Super(Initializer)
{
    auto ResetTempSettings = [](FExportPatchSettings& InTempPatchSetting)
    {
        InTempPatchSetting = FExportPatchSettings{};
        InTempPatchSetting.bByBaseVersion=false;
        // TempPatchSetting.bStorageAssetDependencies = false;
        InTempPatchSetting.bStorageDiffAnalysisResults=false;
        InTempPatchSetting.bStorageDeletedAssetsToNewReleaseJson = false;
        InTempPatchSetting.bStorageConfig = false;
        InTempPatchSetting.bStorageNewRelease = false;
        InTempPatchSetting.bStoragePakFileInfo = false;
        InTempPatchSetting.bCookPatchAssets = true;
        InTempPatchSetting.CookShaderOptions.bSharedShaderLibrary = false;
        InTempPatchSetting.CookShaderOptions.bNativeShader = false;
        InTempPatchSetting.EncryptSettings.bUseDefaultCryptoIni = true;
        InTempPatchSetting.SavePath.Path = TEXT("[PROJECTDIR]/Saved/HotPatcher/Paks");
    };
    ResetTempSettings(TempPatchSetting);
}

#undef LOCTEXT_NAMESPACE