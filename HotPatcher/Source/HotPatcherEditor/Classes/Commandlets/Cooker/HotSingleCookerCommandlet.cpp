#include "HotSingleCookerCommandlet.h"
#include "Commandlets/CommandletHelper.hpp"
// engine header
#include "CoreMinimal.h"
#include "Cooker/MultiCooker/FMultiCookerSettings.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotSingleCookerCommandlet);

int32 UHotSingleCookerCommandlet::Main(const FString& Params)
{
	UE_LOG(LogHotSingleCookerCommandlet, Display, TEXT("UHotSingleCookerCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotSingleCookerCommandlet, Warning, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	if (bStatus && !FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotSingleCookerCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}
	// if(IsRunningCommandlet())
	// {
	// 	// load asset registry
	// 	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	// 	AssetRegistryModule.Get().SearchAllAssets(true);
	// }

	TSharedPtr<FSingleCookerSettings> ExportSingleCookerSetting = MakeShareable(new FSingleCookerSettings);
	
	FString JsonContent;
	if (FPaths::FileExists(config_path) && FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportSingleCookerSetting);
	}

	TMap<FString, FString> KeyValues = UFlibPatchParserHelper::GetCommandLineParamsMap(Params);
	UFlibPatchParserHelper::ReplaceProperty(*ExportSingleCookerSetting, KeyValues);
	
	FString FinalConfig;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(*ExportSingleCookerSetting,FinalConfig);
	UE_LOG(LogHotSingleCookerCommandlet, Display, TEXT("%s"), *FinalConfig);
		
	USingleCookerProxy* SingleCookerProxy = NewObject<USingleCookerProxy>();
	SingleCookerProxy->AddToRoot();
	SingleCookerProxy->SetProxySettings(ExportSingleCookerSetting.Get());
	bool bExportStatus = SingleCookerProxy->DoExport();

	UE_LOG(LogHotSingleCookerCommandlet,Display,TEXT("Single Cook Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return bExportStatus ? 0 : -1;
}
