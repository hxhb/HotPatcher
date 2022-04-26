#pragma once

#include "CoreMinimal.h"
#include "FHotPatcherContextBase.h"
#include "FPatchersModeContext.generated.h"

UENUM(BlueprintType)
enum class EHotPatcherActionModes:uint8
{
	ByPatch,
	ByRelease,
	ByShaderPatch,
	ByGameFeature,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EHotPatcherActionModes, EHotPatcherActionModes::Count);


struct HOTPATCHEREDITOR_API FPatchersModeContext: public FHotPatcherContextBase
{
public:
	virtual ~FPatchersModeContext(){};
	virtual FName GetContextName()const override{ return TEXT("Patch"); }
};