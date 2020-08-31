#pragma once

#include "Commandlets/Commandlet.h"
#include "HotReleaseCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotReleaseCommandlet, Log, All);

UCLASS()
class UHotReleaseCommandlet :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};