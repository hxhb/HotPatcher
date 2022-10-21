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
    TArray<FPakExternalInfo> PakExternalConfigs;
    UPROPERTY(EditAnywhere, config, Category = "Preset")
    TArray<FExportPatchSettings> PresetConfigs;

    UPROPERTY(EditAnywhere, config, Category = "Preview")
    bool bPreviewTooltips = false;
    UPROPERTY(EditAnywhere, config, Category = "Preview")
    bool bExternalFilesCheck = true;

    UPROPERTY(EditAnywhere, config, Category = "CookCommandlet")
    bool bUseHPL = false;
    UPROPERTY(EditAnywhere, config, Category = "CookCommandlet",meta = (RelativeToGameContentDir, LongPackageName,EditCondition="bUseHPL"))
    TArray<FDirectoryPath> SearchPaths;
    UPROPERTY(EditAnywhere, config, Category = "CookCommandlet",meta=(EditCondition="bUseHPL"))
    bool bCopyToStagedBuilds = true;
    // Relative to the Saved directory
    UPROPERTY(EditAnywhere, config, Category = "CookCommandlet",meta=(EditCondition="bUseHPL"))
    FString TempStagedBuildsDir;
    UPROPERTY(EditAnywhere, config, Category = "CookCommandlet",meta=(EditCondition="bUseHPL"))
    FExportPatchSettings HPLPakSettings;
    TArray<FString> GetHPLSearchPaths();
};

FORCEINLINE_DEBUGGABLE TArray<FString> UHotPatcherSettings::GetHPLSearchPaths()
{
    UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
    TArray<FString> SearchPathStrs;
    for(const auto& Path:Settings->SearchPaths)
    {
        SearchPathStrs.Add(Path.Path);
    }
    return SearchPathStrs;
}

FORCEINLINE FString UHotPatcherSettings::GetTempSavedDir()const
{
    return UFlibPatchParserHelper::ReplaceMark(TempPatchSetting.SavePath.Path);
}
FORCEINLINE FString UHotPatcherSettings::GetHPLSavedDir()const
{
    return UFlibPatchParserHelper::ReplaceMark(HPLPakSettings.SavePath.Path);
}

FORCEINLINE UHotPatcherSettings::UHotPatcherSettings(const FObjectInitializer& Initializer):Super(Initializer)
{
    auto ResetTempSettings = [](FExportPatchSettings& InTempPatchSetting)
    {
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
    ResetTempSettings(HPLPakSettings);
    HPLPakSettings.bStandaloneMode = false;
    HPLPakSettings.PakNameRegular = 
    HPLPakSettings.SavePath.Path = TEXT("[PROJECTDIR]/Saved/HotPatcher/HPL");
    TempStagedBuildsDir = TEXT("[PROJECTDIR]/Saved/HotPatcher/TempStagedBuilds");
}

#undef LOCTEXT_NAMESPACE