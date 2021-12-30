#pragma once
// project header
#include "ETargetPlatform.h"

// engine header
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/EngineTypes.h"
#include "FPlatformBasePak.generated.h"

USTRUCT(BlueprintType)
struct FPlatformBasePak
{
    GENERATED_BODY()
    FORCEINLINE FPlatformBasePak()=default;
	
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    ETargetPlatform Platform = ETargetPlatform::None;
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    TArray<FFilePath> Paks;
};