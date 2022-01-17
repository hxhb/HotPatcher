// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#pragma once

#include "HotPatcherSettings.h"
#include "ETargetPlatform.h"
#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherSettings.h"
#include "Modules/ModuleManager.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/FExportReleaseSettings.h"
// engine header
#include "CoreMinimal.h"

extern bool GCookLog;

DECLARE_MULTICAST_DELEGATE_TwoParams(FNotificationEvent,FText,const FString&)


class FHotPatcherCoreModule : public IModuleInterface
{
public:
	static FHotPatcherCoreModule& Get();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
