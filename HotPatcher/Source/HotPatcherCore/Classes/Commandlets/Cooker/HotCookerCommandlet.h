#pragma once

#include "Commandlets/Commandlet.h"
#include "CommandletBase/HotPatcherCommandletBase.h"
#include "HotCookerCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotCookerCommandlet, Log, All);

UCLASS()
class UHotCookerCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
	virtual FString GetCmdletName()const override{ return TEXT("OriginalCookerCmdlet"); }
};