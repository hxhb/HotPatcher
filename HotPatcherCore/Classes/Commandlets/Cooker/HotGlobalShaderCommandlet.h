#pragma once

#include "HotPatcherCommandletBase.h"
#include "Commandlets/Commandlet.h"
#include "HotGlobalShaderCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotGlobalShaderCommandlet, All, All);

UCLASS()
class UHotGlobalShaderCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params)override;
	virtual FString GetCmdletName()const override{ return TEXT("GlobalShaderCmdlet"); }
};