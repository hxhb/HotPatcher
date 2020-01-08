#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "FPatcherSpecifyAsset.generated.h"

USTRUCT(BlueprintType)
struct FPatcherSpecifyAsset
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FSoftObjectPath Asset;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
		bool bAnalysisAssetDependencies;

};
