#include "HotPatcherCookerCommandlet.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

#define COOKER_CONFIG_PARAM_NAME TEXT("-config=")

int32 UHotPatcherCookerCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Log, TEXT("UHotPatcherCookerCommandlet::Main"));
	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(COOKER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogTemp, Error, TEXT("UHotPatcherCookerCommandlet error: not -config=xxxx.json params."));
		return -1;
	}
	
	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogTemp, Error, TEXT("UHotPatcherCookerCommandlet error: cofnig file %s not exists."),*config_path);
		return -1;
	}



	return 0;
}
