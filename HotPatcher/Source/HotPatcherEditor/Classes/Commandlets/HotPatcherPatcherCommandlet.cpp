#include "HotPatcherPatcherCommandlet.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotPatcherPatcher);

#define PATCHER_CONFIG_PARAM_NAME TEXT("-config=")
int32 UHotPatcherPatcherCommandlet::Main(const FString& Params)
{
	UE_LOG(LogHotPatcherPatcher, Display, TEXT("UHotPatcherPatcherCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotPatcherPatcher, Error, TEXT("UHotPatcherPatcherCommandlet error: not -config=xxxx.json params."));
		return -1;
	}

	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotPatcherPatcher, Error, TEXT("UHotPatcherPatcherCommandlet error: cofnig file %s not exists."), *config_path);
		return -1;
	}

	return 0;
}
