#pragma once

#include "Commandlets/Commandlet.h"
#include "HotPatcherCookerCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCooker, Log, All);

UCLASS()
class UHotPatcherCookerCommandlet :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};