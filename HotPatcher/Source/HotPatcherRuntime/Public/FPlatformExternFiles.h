#pragma once
// project header
#include "AssetManager/FAssetDependenciesInfo.h"
#include "FPatcherSpecifyAsset.h"
#include "FExternFileInfo.h"
#include "ETargetPlatform.h"

// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "FPlatformExternFiles.generated.h"

USTRUCT(BlueprintType)
struct FPlatformExternFiles
{
    GENERATED_BODY()
    FORCEINLINE FPlatformExternFiles()=default;
    FORCEINLINE FPlatformExternFiles(ETargetPlatform InPlatform,const TArray<FExternFileInfo>& InFiles):
        Platform(InPlatform),ExternFiles(InFiles){}
	
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    ETargetPlatform Platform = ETargetPlatform::None;
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    TArray<FExternFileInfo> ExternFiles;
};