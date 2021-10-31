// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Features/IModularFeature.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Misc/EnumRange.h"
#include "BinariesPatchFeature.generated.h"

#define BINARIES_DIFF_PATCH_FEATURE_NAME TEXT("BinariesDiffPatchFeatures")

UENUM(BlueprintType)
enum class EBinariesPatchFeature:uint8
{
	None,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EBinariesPatchFeature, EBinariesPatchFeature::Count);

struct IBinariesDiffPatchFeature: public IModularFeature
{
	virtual ~IBinariesDiffPatchFeature(){};
	virtual bool CreateDiff(const TArray<uint8>& NewData, const TArray<uint8>& OldData, TArray<uint8>& OutPatch) = 0;
	virtual bool PatchDiff(const TArray<uint8>& OldData, const TArray<uint8>& PatchData, TArray<uint8>& OutNewData) = 0;
	virtual FString GetFeatureName()const = 0;
};

class FBinariesPatchFeatureModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};