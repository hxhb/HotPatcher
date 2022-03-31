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
	virtual FName GetContextName()const override{ return TEXT("Patch"); }
	FORCEINLINE_DEBUGGABLE void SetPatcherMode(EHotPatcherActionModes InPatcherMode){ PatcherMode = InPatcherMode; }
	FORCEINLINE_DEBUGGABLE EHotPatcherActionModes GetMode() { return PatcherMode; }
	
	FORCEINLINE_DEBUGGABLE virtual void SetModeByName(FName InPatcherModeName) override
	{
		EHotPatcherActionModes tempMode;
		if(THotPatcherTemplateHelper::GetEnumValueByName(InPatcherModeName.ToString(),tempMode))
		{
			SetPatcherMode(tempMode);
		}
	}
	
	FORCEINLINE_DEBUGGABLE virtual FName GetModeName() override
	{
		return *THotPatcherTemplateHelper::GetEnumNameByValue(GetMode(),false);
	}

private:

	EHotPatcherActionModes PatcherMode;
};