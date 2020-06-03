#pragma once

#include "Commandlets/Commandlet.h"
#include "HotCookerCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotCookerCommandlet, Log, All);

UCLASS()
class UHotCookerCommandlet :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};