#pragma once

#include "Commandlets/Commandlet.h"
#include "HotPatcherPatcherCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherPatcher, Log, All);

UCLASS()
class UHotPatcherPatcherCommandlet :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};