#pragma once

#include "ETargetPlatform.h"
#include "FExternFileInfo.h"
#include "FExternDirectoryInfo.h"

// engine heacer
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FPlatformExternAssets.generated.h"


USTRUCT(BlueprintType)
struct FPlatformExternAssets
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
    ETargetPlatform TargetPlatform = ETargetPlatform::None;

    UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
    TArray<FExternFileInfo> AddExternFileToPak;
    UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="")
    TArray<FExternDirectoryInfo> AddExternDirectoryToPak;


    bool operator==(const FPlatformExternAssets& R)const
    {
        return TargetPlatform == R.TargetPlatform;
    }
};


