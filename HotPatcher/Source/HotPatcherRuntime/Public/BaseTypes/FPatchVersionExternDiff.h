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
	UPROPERTY(EditAnywhere)
	ETargetPlatform Platform {ETargetPlatform::None};
	UPROPERTY(EditAnywhere)
	TArray<FExternFileInfo> AddExternalFiles{};
	UPROPERTY(EditAnywhere)
	TArray<FExternFileInfo> ModifyExternalFiles{};
	UPROPERTY(EditAnywhere)
	TArray<FExternFileInfo> DeleteExternalFiles{};
};