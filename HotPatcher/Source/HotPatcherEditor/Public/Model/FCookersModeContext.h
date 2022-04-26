#pragma once

#include "CoreMinimal.h"
#include "FHotPatcherContextBase.h"
#include "Templates/HotPatcherTemplateHelper.hpp"
#include "FCookersModeContext.generated.h"

UENUM(BlueprintType)
enum class EHotPatcherCookActionMode:uint8
{
	ByOriginal,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EHotPatcherCookActionMode, EHotPatcherCookActionMode::Count);

struct HOTPATCHEREDITOR_API FCookersModeContext:public FHotPatcherContextBase
{
public:
	virtual FName GetContextName()const override{ return TEXT("Cooker"); }
};