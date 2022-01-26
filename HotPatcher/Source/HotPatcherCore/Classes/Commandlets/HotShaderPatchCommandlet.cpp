#include "HotShaderPatchCommandlet.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"
#include "CommandletHelper.h"
// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"
#include "ShaderPatch/ShaderPatchProxy.h"

DEFINE_LOG_CATEGORY(LogHotShaderPatchCommandlet);

int32 UHotShaderPatchCommandlet::Main(const FString& Params)
{
	Super::Main(Params);
	UE_LOG(LogHotShaderPatchCommandlet, Display, TEXT("UHotShaderPatchCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotShaderPatchCommandlet, Warning, TEXT("not -config=xxxx.json params."));
		return -1;
	}

	if (bStatus && !FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotShaderPatchCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}
	if(IsRunningCommandlet())
	{
		// load asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().SearchAllAssets(true);
	}

	TSharedPtr<FExportShaderPatchSettings> ExportShaderPatchSetting = MakeShareable(new FExportShaderPatchSettings);
	
	FString JsonContent;
	if (FPaths::FileExists(config_path) && FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportShaderPatchSetting);
	}

	TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
	THotPatcherTemplateHelper::ReplaceProperty(*ExportShaderPatchSetting, KeyValues);
	
	FString FinalConfig;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportShaderPatchSetting,FinalConfig);
	UE_LOG(LogHotShaderPatchCommandlet, Display, TEXT("%s"), *FinalConfig);
		
	UShaderPatchProxy* ShaderPatchProxy = NewObject<UShaderPatchProxy>();
	ShaderPatchProxy->AddToRoot();
	ShaderPatchProxy->Init(ExportShaderPatchSetting.Get());
	bool bExportStatus = ShaderPatchProxy->DoExport();

	UE_LOG(LogHotShaderPatchCommandlet,Display,TEXT("Export Shader Patch Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return bExportStatus ? 0 : -1;
}
