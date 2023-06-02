#include "HotSingleCookerCommandlet.h"
#include "CommandletBase/CommandletHelper.h"
// engine header
#include "CoreMinimal.h"
#include "Cooker/MultiCooker/FSingleCookerSettings.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "HAL/ExceptionHandling.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotSingleCookerCommandlet);

int32 UHotSingleCookerCommandlet::Main(const FString& Params)
{
	Super::Main(Params);
	SCOPED_NAMED_EVENT_TEXT("UHotSingleCookerCommandlet::Main",FColor::Red);
	
	UE_LOG(LogHotSingleCookerCommandlet, Display, TEXT("UHotSingleCookerCommandlet::Main"));

	if(CmdConfigPath.IsEmpty())
	{
		return -1;
	}

	ExportSingleCookerSetting = MakeShareable(new FSingleCookerSettings);
	
	FString JsonContent;
	if (FPaths::FileExists(CmdConfigPath) && FFileHelper::LoadFileToString(JsonContent, *CmdConfigPath))
	{
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportSingleCookerSetting);
	}

	TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
	THotPatcherTemplateHelper::ReplaceProperty(*ExportSingleCookerSetting, KeyValues);
	
	CommandletHelper::ModifyTargetPlatforms(Params,TARGET_PLATFORMS_OVERRIDE,ExportSingleCookerSetting->CookTargetPlatforms,true);
	if(ExportSingleCookerSetting->bDisplayConfig)
	{
		FString FinalConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportSingleCookerSetting,FinalConfig);
	}
	
	// if(IsRunningCommandlet() && ExportSingleCookerSetting->bPackageTracker && ExportSingleCookerSetting->bCookPackageTrackerAssets)
	// {
	// 	SCOPED_NAMED_EVENT_TEXT("SearchAllAssets",FColor::Red);
	// 	// load asset registry
	// 	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	// 	AssetRegistryModule.Get().SearchAllAssets(true);
	// }
	
	UE_LOG(LogHotSingleCookerCommandlet, Display, TEXT("Cooker %s Id %d,Assets Num %d"), *ExportSingleCookerSetting->MissionName,ExportSingleCookerSetting->MissionID,ExportSingleCookerSetting->CookAssets.Num());

	GAlwaysReportCrash = false;
	USingleCookerProxy* SingleCookerProxy = NewObject<USingleCookerProxy>();
	SingleCookerProxy->AddToRoot();
	SingleCookerProxy->Init(ExportSingleCookerSetting.Get());
	bool bExportStatus = SingleCookerProxy->DoExport();
	
	CommandletHelper::MainTick([SingleCookerProxy]()->bool
	{
		return SingleCookerProxy->IsFinsihed();
	});
	
	SingleCookerProxy->Shutdown();
	UE_LOG(LogHotSingleCookerCommandlet,Display,TEXT("Single Cook Misstion %s %d is %s!"),*ExportSingleCookerSetting->MissionName,ExportSingleCookerSetting->MissionID,bExportStatus?TEXT("Successed"):TEXT("Failure"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return bExportStatus ? 0 : -1;
}