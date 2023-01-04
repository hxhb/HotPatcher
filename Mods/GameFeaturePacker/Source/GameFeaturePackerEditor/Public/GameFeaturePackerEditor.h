// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HotPatcherActionManager.h"
#include "HotPatcherModBaseModule.h"
#include "Modules/ModuleManager.h"

class FGameFeaturePackerEditorModule : public FHotPatcherModBaseModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual FHotPatcherModDesc GetModDesc()const override;
};
