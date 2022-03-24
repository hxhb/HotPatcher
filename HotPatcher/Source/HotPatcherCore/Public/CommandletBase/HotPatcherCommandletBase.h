#pragma once

#include "Commandlets/Commandlet.h"
#include "HotPatcherCommandletBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCommandletBase, All, All);

UCLASS()
class HOTPATCHERCORE_API UHotPatcherCommandletBase :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};