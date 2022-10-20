#pragma once

#include "HotPatcherCommandletBase.h"
#include "Commandlets/Commandlet.h"
#include "HotPatcherCommandlet.generated.h"

// DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCommandlet, All, All);

UCLASS()
class UHotPatcherCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};