// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#pragma once

#include "HotPatcherSettings.h"
#include "Cooker/MultiCooker/FSingleCookerSettings.h"
// engine header
#include "CoreMinimal.h"

HOTPATCHERCORE_API extern struct FExportPatchSettings* GPatchSettings;
HOTPATCHERCORE_API extern struct FExportReleaseSettings* GReleaseSettings;
extern HOTPATCHERCORE_API bool GCookLog;
extern HOTPATCHERCORE_API FString GToolName;
extern HOTPATCHERCORE_API FString GRemoteVersionFile;
extern HOTPATCHERCORE_API int32 GToolMainVersion;
extern HOTPATCHERCORE_API int32 GToolPatchVersion;

HOTPATCHERCORE_API void ReceiveOutputMsg(class FProcWorkerThread* Worker,const FString& InMsg);

// delegate
DECLARE_MULTICAST_DELEGATE_TwoParams(FNotificationEvent,FText,const FString&)
DECLARE_MULTICAST_DELEGATE_OneParam(FSingleCookerAdditionalWorkerRegister,const TSharedPtr<FSingleCookerSettings>&);


class HOTPATCHERCORE_API FHotPatcherCoreModule : public IModuleInterface
{
public:
	static FHotPatcherCoreModule& Get();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual int32 GetMainVersion()const{ return CURRENT_VERSION_ID; }
	virtual int32 GetPatchVersion()const { return CURRENT_PATCH_ID; }

public:
	FSingleCookerAdditionalWorkerRegister& GetSingleCookerAdditionalPreWorkerRegister(){ return SingleCookerAdditionalPreWorkerRegister; }
	FSingleCookerAdditionalWorkerRegister& GetSingleCookerAdditionalPostWorkerRegister(){ return SingleCookerAdditionalPostWorkerRegister; }
private:
	FSingleCookerAdditionalWorkerRegister SingleCookerAdditionalPreWorkerRegister;
	FSingleCookerAdditionalWorkerRegister SingleCookerAdditionalPostWorkerRegister;
};
