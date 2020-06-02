#pragma once

#include "Commandlets/Commandlet.h"
#include "HotPatcherPatcherCommandlet.generated.h"


UCLASS()
class UHotPatcherPatcherCommandlet :public UCommandlet
{
	GENERATED_BODY()

public:

	virtual int32 Main(const FString& Params)override;
};