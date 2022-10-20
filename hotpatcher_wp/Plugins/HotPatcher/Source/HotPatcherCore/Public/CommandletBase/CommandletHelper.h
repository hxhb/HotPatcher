#pragma once

#include "ETargetPlatform.h"
#include "FlibPatchParserHelper.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "HotPatcherLog.h"

#define PATCHER_CONFIG_PARAM_NAME TEXT("-config=")
#define ADD_PATCH_PLATFORMS TEXT("AddPatchPlatforms")

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCommandlet, All, All);

namespace CommandletHelper
{
	HOTPATCHERCORE_API void ReceiveMsg(const FString& InMsgType,const FString& InMsg);
	HOTPATCHERCORE_API void ReceiveShowMsg(const FString& InMsg);

	HOTPATCHERCORE_API TArray<FString> ParserPatchConfigByCommandline(const FString& Commandline,const FString& Token);

	HOTPATCHERCORE_API TArray<ETargetPlatform> ParserPlatforms(const FString& Commandline, const FString& Token);

	HOTPATCHERCORE_API TArray<FDirectoryPath> ParserPatchFilters(const FString& Commandline,const FString& FilterName);

	HOTPATCHERCORE_API void MainTick(TFunction<bool()> IsRequestExit);
}
