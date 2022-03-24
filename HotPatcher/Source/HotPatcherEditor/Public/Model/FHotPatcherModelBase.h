#pragma once

#include "CoreMinimal.h"
#include "Templates/HotPatcherTemplateHelper.hpp"


struct HOTPATCHEREDITOR_API FHotPatcherModelBase
{
public:
	FHotPatcherModelBase()=default;
	virtual ~FHotPatcherModelBase(){}
	virtual FName GetModelName()const{ return TEXT(""); }
};