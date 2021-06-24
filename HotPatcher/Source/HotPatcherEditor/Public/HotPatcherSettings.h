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
    
    UPROPERTY(EditAnywhere, config, Category = "Editor|IoStore")
    bool bIoStore = false;
    UPROPERTY(EditAnywhere, config, Category = "Editor|IoStore")
    bool bAllowBulkDataInIoStore = false;
    UPROPERTY(EditAnywhere, config, Category = "Editor|IoStore")
    bool bStorageIoStorePakList = false;
    UPROPERTY(EditAnywhere, config, Category = "Editor|IoStore")
    TArray<FString> IoStorePakListOptions;
    UPROPERTY(EditAnywhere, config, Category = "Editor|IoStore")
    TArray<FString> IoStoreCommandletOptions;
    UPROPERTY(EditAnywhere, config, Category = "Editor|UnrealPak")
    bool bStorageUnrealPakList = false;
    UPROPERTY(EditAnywhere, config, Category = "Editor|UnrealPak")
    TArray<FString> UnrealPakListOptions;
    UPROPERTY(EditAnywhere, config, Category = "Editor|UnrealPak")
    TArray<FString> UnrealPakCommandletOptions;
    
    UPROPERTY(EditAnywhere, config, Category = "Editor|Encrypt")
    FPakEncryptSettings EncryptSettings;
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    TArray<FPakExternalInfo> PakExternalConfigs;
};


#undef LOCTEXT_NAMESPACE