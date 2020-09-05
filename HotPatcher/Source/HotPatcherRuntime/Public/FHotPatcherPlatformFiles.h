#pragma once
// project header
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FPatcherSpecifyAsset.h"
#include "FExternAssetFileInfo.h"
#include "ETargetPlatform.h"

// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "FHotPatcherPlatformFiles.generated.h"

USTRUCT(BlueprintType)
struct FHotPatcherPlatformFiles
{
    GENERATED_BODY()
    FORCEINLINE FHotPatcherPlatformFiles()=default;
    FORCEINLINE FHotPatcherPlatformFiles(ETargetPlatform InPlatform,const TArray<FExternAssetFileInfo>& InFiles):
        Platform(InPlatform),ExternFiles(InFiles){}
	
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    ETargetPlatform Platform;
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    TArray<FExternAssetFileInfo> ExternFiles;
};