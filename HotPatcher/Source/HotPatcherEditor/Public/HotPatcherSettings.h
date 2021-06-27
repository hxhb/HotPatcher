#pragma once
#include "ETargetPlatform.h"
#include "CreatePatch/FExportPatchSettings.h"

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
class HOTPATCHEREDITOR_API UHotPatcherSettings:public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    bool bWhiteListCookInEditor;
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    TArray<ETargetPlatform> PlatformWhitelists;
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    FString TempPakDir = TEXT("Saved/HotPatcher/Paks");
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    bool bUseStandaloneMode;
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    bool bSavePatchConfig;
    UPROPERTY(EditAnywhere, config, Category = "UnrealPak")
    FUnrealPakSettings UnreakPakSettings;
    UPROPERTY(EditAnywhere, config, Category = "IoStore")
    FIoStoreSettings IoStoreSettings;
    UPROPERTY(EditAnywhere, config, Category = "Encrypt")
    FPakEncryptSettings EncryptSettings;

    UPROPERTY(EditAnywhere, config, Category = "Preset")
    TArray<FPakExternalInfo> PakExternalConfigs;
    UPROPERTY(EditAnywhere, config, Category = "Preset")
    TArray<FExportPatchSettings> PresetConfigs;
};


#undef LOCTEXT_NAMESPACE