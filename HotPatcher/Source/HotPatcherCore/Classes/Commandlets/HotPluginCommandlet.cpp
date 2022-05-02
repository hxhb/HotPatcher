#include "HotPluginCommandlet.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "GameFeature/FGameFeaturePackagerSettings.h"
#include "FlibPatchParserHelper.h"

#include "CommandletHelper.h"

// engine header
#include "CoreMinimal.h"
#include "GameFeature/GameFeatureProxy.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotPluginCommandlet);

TSharedPtr<FProcWorkerThread> PluginProc;

int32 UHotPluginCommandlet::Main(const FString& Params)
{
	Super::Main(Params);
	UE_LOG(LogHotPluginCommandlet, Display, TEXT("UHotPluginCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotPluginCommandlet, Error, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotPluginCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}

	FString JsonContent;
	bool bExportStatus = false;
	if (FFileHelper::LoadFileToString(JsonContent, *config_path))
	{

		if(IsRunningCommandlet())
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().SearchAllAssets(true);
		}
		
		TSharedPtr<FGameFeaturePackagerSettings> PluginPackagerSetting = MakeShareable(new FGameFeaturePackagerSettings);
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*PluginPackagerSetting);
		
		TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
		THotPatcherTemplateHelper::ReplaceProperty(*PluginPackagerSetting, KeyValues);
		TArray<ETargetPlatform> AddPlatforms = CommandletHelper::ParserPlatforms(Params,ADD_PATCH_PLATFORMS);

		FString FinalConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*PluginPackagerSetting,FinalConfig);
		UE_LOG(LogHotPluginCommandlet, Display, TEXT("%s"), *FinalConfig);

		
		UGameFeatureProxy* GameFeatureProxy = NewObject<UGameFeatureProxy>();
		GameFeatureProxy->AddToRoot();
		GameFeatureProxy->Init(PluginPackagerSetting.Get());
		GameFeatureProxy->OnPaking.AddStatic(&::CommandletHelper::ReceiveMsg);
		GameFeatureProxy->OnShowMsg.AddStatic(&::CommandletHelper::ReceiveShowMsg);
		bExportStatus = GameFeatureProxy->DoExport();
		
		UE_LOG(LogHotPluginCommandlet,Display,TEXT("Generate Game Feature Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	}

	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return 0;
}
