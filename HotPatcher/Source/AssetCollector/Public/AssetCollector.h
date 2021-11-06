// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Features/IModularFeature.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Misc/EnumRange.h"

class FAssetCollectorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};