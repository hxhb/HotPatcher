#include "HotPatcherCommandlet.h"
// #include "CreatePatch/FExportPatchSettingsEx.h"
#include "CreatePatch/PatcherProxy.h"
#include "CommandletHelper.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotPatcherCommandlet);

int32 UHotPatcherCommandlet::Main(const FString& Params)
{
	Super::Main(Params);

	FCommandLine::Append(TEXT(" -buildmachine"));
	GIsBuildMachine = true;
	
	UE_LOG(LogHotPatcherCommandlet, Display, TEXT("UHotPatcherCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotPatcherCommandlet, Error, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotPatcherCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
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
		
		TSharedPtr<FExportPatchSettings> ExportPatchSetting = MakeShareable(new FExportPatchSettings);
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportPatchSetting);
		
		TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
		THotPatcherTemplateHelper::ReplaceProperty(*ExportPatchSetting, KeyValues);
		TArray<ETargetPlatform> AddPlatforms = CommandletHelper::ParserPatchPlatforms(Params);
	
		if(AddPlatforms.Num())
		{
			for(auto& Platform:AddPlatforms)
			{
				ExportPatchSetting->PakTargetPlatforms.AddUnique(Platform);
			}
		}
		ExportPatchSetting->GetAssetScanConfigRef().AssetIncludeFilters.Append(CommandletHelper::ParserPatchFilters(Params,TEXT("AssetIncludeFilters")));
		ExportPatchSetting->GetAssetScanConfigRef().AssetIgnoreFilters.Append(CommandletHelper::ParserPatchFilters(Params,TEXT("AssetIgnoreFilters")));
		
		FString FinalConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportPatchSetting,FinalConfig);
		UE_LOG(LogHotPatcherCommandlet, Display, TEXT("%s"), *FinalConfig);

		
		UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->AddToRoot();
		PatcherProxy->Init(ExportPatchSetting.Get());
		PatcherProxy->OnPaking.AddStatic(&::CommandletHelper::ReceiveMsg);
		PatcherProxy->OnShowMsg.AddStatic(&::CommandletHelper::ReceiveShowMsg);
		bExportStatus = PatcherProxy->DoExport();
		
		UE_LOG(LogHotPatcherCommandlet,Display,TEXT("Export Patch Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	}

	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return (int32)!bExportStatus;
}
