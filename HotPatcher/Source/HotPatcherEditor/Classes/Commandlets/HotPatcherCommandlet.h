#pragma once

#include "Commandlets/Commandlet.h"
#include "HotPatcherCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCommandlet, Log, All);

UCLASS()
class UHotPatcherCommandlet :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};