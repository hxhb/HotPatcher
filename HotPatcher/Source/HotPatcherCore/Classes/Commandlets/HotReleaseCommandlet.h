#pragma once
#include "HotPatcherCommandletBase.h"
#include "Commandlets/Commandlet.h"
#include "HotReleaseCommandlet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotReleaseCommandlet, All, All);

UCLASS()
class UHotReleaseCommandlet :public UHotPatcherCommandletBase
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};