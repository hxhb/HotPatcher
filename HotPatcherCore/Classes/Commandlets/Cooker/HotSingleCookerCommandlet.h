#pragma once

#include "Commandlets/Commandlet.h"
#include "CommandletBase/HotPatcherCommandletBase.h"
#include "Cooker/MultiCooker/FSingleCookerSettings.h"
#include "HotSingleCookerCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotSingleCookerCommandlet, All, All);

UCLASS()
class UHotSingleCookerCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
	virtual FString GetCmdletName()const override
	{
		FString Result = TEXT("SingleCookerCmdlet");
		if(ExportSingleCookerSetting.IsValid())
		{
			Result = FString::Printf(TEXT("%s_%s"),*Result,*ExportSingleCookerSetting.Get()->MissionName);
		}
		return Result;
	}
protected:
	TSharedPtr<FSingleCookerSettings> ExportSingleCookerSetting;
};