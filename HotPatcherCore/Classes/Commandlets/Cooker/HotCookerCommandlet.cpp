#include "HotCookerCommandlet.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "FCookerConfig.h"
#include "FlibPatchParserHelper.h"
#include "CommandletBase/CommandletHelper.h"

// / engine header
#include "CoreMinimal.h"
#include "HotPatcherCore.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"

#define COOKER_CONFIG_PARAM_NAME TEXT("-config=")

DEFINE_LOG_CATEGORY(LogHotCookerCommandlet);

TSharedPtr<FProcWorkerThread> CookerProc;

int32 UHotCookerCommandlet::Main(const FString& Params)
{
	Super::Main(Params);
	UE_LOG(LogHotCookerCommandlet, Display, TEXT("UHotCookerCommandlet::Main"));
	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(COOKER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotCookerCommandlet, Error, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotCookerCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}

	FString JsonContent;
	if (FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		UE_LOG(LogHotCookerCommandlet, Display, TEXT("%s"), *JsonContent);
		FCookerConfig CookConfig;
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,CookConfig);
		
		TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
		THotPatcherTemplateHelper::ReplaceProperty(CookConfig, KeyValues);
		
		if (CookConfig.bCookAllMap)
		{
			CookConfig.CookMaps = UFlibPatchParserHelper::GetAvailableMaps(UKismetSystemLibrary::GetProjectDirectory(), ENABLE_COOK_ENGINE_MAP, ENABLE_COOK_PLUGIN_MAP, true);
		}
		FString CookCommand;
		UFlibPatchParserHelper::GetCookProcCommandParams(CookConfig, CookCommand);

		UE_LOG(LogHotCookerCommandlet, Display, TEXT("CookCommand:%s %s"), *CookConfig.EngineBin,*CookCommand);

		if (FPaths::FileExists(CookConfig.EngineBin) && FPaths::FileExists(CookConfig.ProjectPath))
		{
			CookerProc = MakeShareable(new FProcWorkerThread(TEXT("CookThread"), CookConfig.EngineBin, CookCommand));
			CookerProc->ProcOutputMsgDelegate.BindStatic(&::ReceiveOutputMsg);

			CookerProc->Execute();
			CookerProc->Join();
		}
	}
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	return 0;
}
