#pragma once
#include "ETargetPlatform.h"
#include "HotPatcherEditor.h"

#include "CoreMinimal.h"
#include "Kismet/KismetTextLibrary.h"
#include "HotPatcherSettings.generated.h"
#define LOCTEXT_NAMESPACE "UHotPatcherSettings"

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
    TArray<FString> CookParams;
    UPROPERTY(EditAnywhere, config, Category = "Editor")
    FString TempPakDir = TEXT("Saved/HotPatcher/Paks");
};


#undef LOCTEXT_NAMESPACE