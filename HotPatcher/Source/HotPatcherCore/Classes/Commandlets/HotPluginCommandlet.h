#pragma once

#include "HotPatcherCommandletBase.h"
#include "Commandlets/Commandlet.h"
#include "HotPluginCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotPluginCommandlet, Log, All);

UCLASS()
class UHotPluginCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};