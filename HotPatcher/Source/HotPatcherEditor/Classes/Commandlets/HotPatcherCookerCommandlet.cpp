#include "HotPatcherCookerCommandlet.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "FCookerConfig.h"
#include "FlibPatchParserHelper.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

#define COOKER_CONFIG_PARAM_NAME TEXT("-config=")

DEFINE_LOG_CATEGORY(LogHotPatcherCooker);

TSharedPtr<FProcWorkerThread> CookerProc;

void ReceiveOutputMsg(const FString& InMsg)
{
	FString FindItem(TEXT("Display:"));
	int32 Index= InMsg.Len() - InMsg.Find(FindItem)- FindItem.Len();
	if (InMsg.Contains(TEXT("Error:")))
	{
		UE_LOG(LogHotPatcherCooker, Error, TEXT("%s"), *InMsg);
	}
	else if (InMsg.Contains(TEXT("Warning:")))
	{
		UE_LOG(LogHotPatcherCooker, Warning, TEXT("%s"), *InMsg);
	}
	else
	{
		UE_LOG(LogHotPatcherCooker, Display, TEXT("%s"), *InMsg.Right(Index));
	}
}

int32 UHotPatcherCookerCommandlet::Main(const FString& Params)
{
	UE_LOG(LogHotPatcherCooker, Display, TEXT("UHotPatcherCookerCommandlet::Main"));
	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(COOKER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotPatcherCooker, Error, TEXT("UHotPatcherCookerCommandlet error: not -config=xxxx.json params."));
		return -1;
	}

	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotPatcherCooker, Error, TEXT("UHotPatcherCookerCommandlet error: cofnig file %s not exists."), *config_path);
		return -1;
	}

	FString JsonContent;
	if (FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		FCookerConfig CookConfig = UFlibPatchParserHelper::DeSerializeCookerConfig(JsonContent);

		FString CookCommand;
		UFlibPatchParserHelper::GetCookProcCommandParams(CookConfig, CookCommand);

		UE_LOG(LogHotPatcherCooker, Display, TEXT("CookCommand:%s %s"), *CookConfig.EngineBin,*CookCommand);

		if (FPaths::FileExists(CookConfig.EngineBin) && FPaths::FileExists(CookConfig.ProjectPath))
		{
			CookerProc = MakeShareable(new FProcWorkerThread(TEXT("CookThread"), CookConfig.EngineBin, CookCommand));
			CookerProc->ProcOutputMsgDelegate.AddStatic(&::ReceiveOutputMsg);

			CookerProc->Execute();
			CookerProc->Join();
		}
	}
	system("pause");
	return 0;
}
