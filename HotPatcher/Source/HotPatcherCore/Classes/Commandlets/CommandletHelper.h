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
	void ReceiveMsg(const FString& InMsgType,const FString& InMsg);
	void ReceiveShowMsg(const FString& InMsg);

	TArray<FString> ParserPatchConfigByCommandline(const FString& Commandline,const FString& Token);

	TArray<ETargetPlatform> ParserPatchPlatforms(const FString& Commandline);

	TArray<FDirectoryPath> ParserPatchFilters(const FString& Commandline,const FString& FilterName);

	void MainTick(TFunction<bool()> IsRequestExit);
}
