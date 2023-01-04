// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HotPatcherActionManager.h"
#include "HotPatcherModBaseModule.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShaderPatcherEditor, All, All);

class FShaderPatcherEditorModule : public FHotPatcherModBaseModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual FHotPatcherModDesc GetModDesc()const override;
};
