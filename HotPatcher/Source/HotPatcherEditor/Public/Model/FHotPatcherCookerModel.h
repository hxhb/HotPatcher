#pragma once

#include "CoreMinimal.h"
#include "FHotPatcherModelBase.h"
#include "Templates/HotPatcherTemplateHelper.hpp"
#include "FHotPatcherCookerModel.generated.h"

UENUM(BlueprintType)
enum class EHotPatcherCookActionMode:uint8
{
	ByOriginal,
	Count UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EHotPatcherCookActionMode, EHotPatcherCookActionMode::Count);

struct HOTPATCHEREDITOR_API FHotPatcherCookerModel:public FHotPatcherModelBase
{
public:
	virtual FName GetModelName()const override{ return TEXT("Cooker"); }
	
	void SetCookerMode(EHotPatcherCookActionMode InCookerMode)
	{
		CookerMode = InCookerMode;
	}
	void SetCookerModeByName(FName CookerModeEnumName)
	{
		EHotPatcherCookActionMode tempCookerMode;
		if(THotPatcherTemplateHelper::GetEnumValueByName(CookerModeEnumName.ToString(),tempCookerMode))
		{
			SetCookerMode(tempCookerMode);
		}
	}
	EHotPatcherCookActionMode GetCookerMode()
	{
		return CookerMode;
	}
	FString GetCookerModeName()
	{
		return THotPatcherTemplateHelper::GetEnumNameByValue(CookerMode,false);
	}

private:

	EHotPatcherCookActionMode CookerMode;
};