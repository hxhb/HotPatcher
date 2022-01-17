#include "HotMultiCookerCommandlet.h"
// engine header
#include "CoreMinimal.h"
#include "Commandlets/CommandletHelper.hpp"
#include "Cooker/MultiCooker/FMultiCookerSettings.h"
#include "Cooker/MultiCooker/MultiCookerProxy.h"
#include "HAL/ExceptionHandling.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotMultiCookerCommandlet);

int32 UHotMultiCookerCommandlet::Main(const FString& Params)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("MultiCookCommandlet::Main"),FColor::Red);
	UE_LOG(LogHotMultiCookerCommandlet, Display, TEXT("UHotMultiCookerCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotMultiCookerCommandlet, Warning, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	if (bStatus && !FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotMultiCookerCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}

	if(IsRunningCommandlet())
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("SearchAllAssets"),FColor::Red);
		// load asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().SearchAllAssets(true);
	}

	TSharedPtr<FMultiCookerSettings> ExportMultiCookerSetting = MakeShareable(new FMultiCookerSettings);
	
	FString JsonContent;
	if (FPaths::FileExists(config_path) && FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportMultiCookerSetting);
	}

	TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
	THotPatcherTemplateHelper::ReplaceProperty(*ExportMultiCookerSetting, KeyValues);

	if(ExportMultiCookerSetting->bDisplayMissionConfig)
	{
		FString FinalConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportMultiCookerSetting,FinalConfig);
		UE_LOG(LogHotMultiCookerCommandlet, Display, TEXT("%s"), *FinalConfig);
	}
	GAlwaysReportCrash = false;
	UMultiCookerProxy* MultiCookerProxy = NewObject<UMultiCookerProxy>();
	MultiCookerProxy->AddToRoot();
	MultiCookerProxy->Init(ExportMultiCookerSetting.Get());
	bool bExportStatus = MultiCookerProxy->DoExport();

	if(IsRunningCommandlet())
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("wait Cook mission is completed"),FColor::Red);
		MultiCookerProxy->WaitMissionFinished();
	}

	MultiCookerProxy->Shutdown();
	UE_LOG(LogHotMultiCookerCommandlet,Display,TEXT("Multi Process Cook Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return bExportStatus ? 0 : -1;
}
