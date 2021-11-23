#pragma once

#include "Commandlets/Commandlet.h"
#include "HotSingleCookerCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotSingleCookerCommandlet, All, All);

UCLASS()
class UHotSingleCookerCommandlet :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};