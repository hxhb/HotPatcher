#include "HotSingleCookerCommandlet.h"
#include "CommandletBase/CommandletHelper.h"
#include "HotPatcherCore.h"
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
	
	bool bExportStatus = false;
	{
		FScopedNamedEventStatic ScopedNamedEvent_ForCooker(FColor::Blue,*ExportSingleCookerSetting->MissionName);
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
		// for pre additional worker
		if(ExportSingleCookerSetting->bAllowRegisteAdditionalWorker)
		{
			FHotPatcherCoreModule::Get().GetSingleCookerAdditionalPreWorkerRegister().Broadcast(ExportSingleCookerSetting);
			GEngine->ForceGarbageCollection(false);
			CollectGarbage(RF_NoFlags, false);
		}
		GAlwaysReportCrash = false;
		USingleCookerProxy* SingleCookerProxy = NewObject<USingleCookerProxy>();
		SingleCookerProxy->AddToRoot();
		SingleCookerProxy->Init(ExportSingleCookerSetting.Get());
		bExportStatus = SingleCookerProxy->DoExport();
		
		CommandletHelper::MainTick([SingleCookerProxy]()->bool
		{
			return SingleCookerProxy->IsFinsihed();
		});
		
		SingleCookerProxy->Shutdown();
		UE_LOG(LogHotSingleCookerCommandlet,Display,TEXT("Single Cook Mission %s %d is %s!"),*ExportSingleCookerSetting->MissionName,ExportSingleCookerSetting->MissionID,bExportStatus?TEXT("Successed"):TEXT("Failure"));

		// for post additional worker
		if(ExportSingleCookerSetting->bAllowRegisteAdditionalWorker)
		{
			FHotPatcherCoreModule::Get().GetSingleCookerAdditionalPostWorkerRegister().Broadcast(ExportSingleCookerSetting);
			GEngine->ForceGarbageCollection(false);
			CollectGarbage(RF_NoFlags, false);
		}
	}
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return bExportStatus ? 0 : -1;
}