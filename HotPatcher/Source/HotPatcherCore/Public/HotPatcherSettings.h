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
    
    UPROPERTY(EditAnywhere, config, Category = "ConfigTemplate")
    FExportPatchSettings TempPatchSetting;
    
    UPROPERTY(EditAnywhere, config, Category = "Preset")
    TArray<FPakExternalInfo> PakExternalConfigs;
    UPROPERTY(EditAnywhere, config, Category = "Preset")
    TArray<FExportPatchSettings> PresetConfigs;

    UPROPERTY(EditAnywhere, config, Category = "Preview")
    bool bPreviewTooltips = false;
    UPROPERTY(EditAnywhere, config, Category = "Preview")
    bool bExternalFilesCheck = true;
};

FORCEINLINE FString UHotPatcherSettings::GetTempSavedDir()const
{
    return UFlibPatchParserHelper::ReplaceMark(TempPatchSetting.SavePath.Path);
}

FORCEINLINE UHotPatcherSettings::UHotPatcherSettings(const FObjectInitializer& Initializer):Super(Initializer)
{
    TempPatchSetting.bByBaseVersion=false;
    // TempPatchSetting.bStorageAssetDependencies = false;
    TempPatchSetting.bStorageDiffAnalysisResults=false;
    TempPatchSetting.bStorageDeletedAssetsToNewReleaseJson = false;
    TempPatchSetting.bStorageConfig = false;
    TempPatchSetting.bStorageNewRelease = false;
    TempPatchSetting.bStoragePakFileInfo = false;
    TempPatchSetting.bCookPatchAssets = true;
    TempPatchSetting.CookShaderOptions.bSharedShaderLibrary = false;
    TempPatchSetting.CookShaderOptions.bNativeShader = false;
    TempPatchSetting.EncryptSettings.bUseDefaultCryptoIni = true;
    TempPatchSetting.SavePath.Path = TEXT("[PROJECTDIR]/Saved/HotPatcher/Paks");
}

#undef LOCTEXT_NAMESPACE