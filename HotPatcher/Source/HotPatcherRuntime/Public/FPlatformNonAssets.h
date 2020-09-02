#pragma once

#include "ETargetPlatform.h"
#include "FExternAssetFileInfo.h"
#include "FExternDirectoryInfo.h"

// engine heacer
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FPlatformNonAssets.generated.h"


USTRUCT(BlueprintType)
struct FPlatformNonAssets
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    ETargetPlatform TargetPlatform;

    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    TArray<FExternAssetFileInfo> AddExternFileToPak;
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
};


