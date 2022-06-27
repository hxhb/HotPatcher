#pragma once
// project header
#include "BaseTypes/AssetManager/FAssetDependenciesInfo.h"
#include "FExternFileInfo.h"

// engine header
#include "CoreMinimal.h"

#include "FPlatformExternAssets.h"
#include "Engine/EngineTypes.h"
#include "FPatchVersionExternDiff.generated.h"

USTRUCT(BlueprintType)
struct FPatchVersionExternDiff
{
	GENERATED_USTRUCT_BODY()
	FPatchVersionExternDiff()=default;
public:
	UPROPERTY(EditAnywhere, Category="")
	ETargetPlatform Platform {ETargetPlatform::None};
	UPROPERTY(EditAnywhere, Category="")
	TArray<FExternFileInfo> AddExternalFiles{};
	UPROPERTY(EditAnywhere, Category="")
	TArray<FExternFileInfo> ModifyExternalFiles{};
	UPROPERTY(EditAnywhere, Category="")
	TArray<FExternFileInfo> DeleteExternalFiles{};
};
