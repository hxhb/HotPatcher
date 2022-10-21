// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#pragma once

#include "HotPatcherSettings.h"
// engine header
#include "CoreMinimal.h"

extern HOTPATCHERCORE_API bool GCookLog;
extern HOTPATCHERCORE_API FString GToolName;
extern HOTPATCHERCORE_API FString GRemoteVersionFile;
extern HOTPATCHERCORE_API int32 GToolMainVersion;
extern HOTPATCHERCORE_API int32 GToolPatchVersion;

HOTPATCHERCORE_API void ReceiveOutputMsg(class FProcWorkerThread* Worker,const FString& InMsg);

DECLARE_MULTICAST_DELEGATE_TwoParams(FNotificationEvent,FText,const FString&)


class FHotPatcherCoreModule : public IModuleInterface
{
public:
	static FHotPatcherCoreModule& Get();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
private:
	virtual void OnPreEngineExit_Commandlet();
	void SetHPLStagedBuildsDirConfig();
};
