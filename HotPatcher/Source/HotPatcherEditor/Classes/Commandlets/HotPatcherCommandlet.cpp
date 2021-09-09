#include "HotPatcherCommandlet.h"
// #include "CreatePatch/FExportPatchSettingsEx.h"
#include "CreatePatch/PatcherProxy.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotPatcherCommandlet);

#define PATCHER_CONFIG_PARAM_NAME TEXT("-config=")

namespace NSPatch
{
	void ReceiveMsg(const FString& InMsgType,const FString& InMsg)
	{
		UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s:%s"),*InMsgType,*InMsg);
	}

	void ReceiveShowMsg(const FString& InMsg)
	{
		UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s"),*InMsg);
	}
}

TArray<FString> ParserPatchConfigByCommandline(const FString& Commandline,const FString& Token)
{
	TArray<FString> result;
	TMap<FString, FString> KeyValues = UFlibPatchParserHelper::GetCommandLineParamsMap(Commandline);
	if(KeyValues.Find(Token))
	{
		FString AddPakListInfo = *KeyValues.Find(Token);
		AddPakListInfo.ParseIntoArray(result,TEXT(","));
	}
	return result;
}


#define ADD_PATCH_PLATFORMS TEXT("AddPatchPlatforms")
TArray<ETargetPlatform> ParserPatchPlatforms(const FString& Commandline)
{
	TArray<ETargetPlatform> result;
	for(auto& PlatformName:ParserPatchConfigByCommandline(Commandline,ADD_PATCH_PLATFORMS))
	{
		ETargetPlatform Platform = ETargetPlatform::None;
		UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
		if(Platform != ETargetPlatform::None)
		{
			result.AddUnique(Platform);
		}
	}
	return result;
}

TArray<FDirectoryPath> ParserPatchFilters(const FString& Commandline,const FString& FilterName)
{
	TArray<FDirectoryPath> Result;
	for(auto& FilterPath:ParserPatchConfigByCommandline(Commandline,FString::Printf(TEXT("Add%s"),*FilterName)))
	{
		FDirectoryPath Path;
		Path.Path = FilterPath;
		Result.Add(Path);
	}
	return Result;
}

int32 UHotPatcherCommandlet::Main(const FString& Params)
{
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
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportPatchSetting);
		
		TMap<FString, FString> KeyValues = UFlibPatchParserHelper::GetCommandLineParamsMap(Params);
		UFlibPatchParserHelper::ReplaceProperty(*ExportPatchSetting, KeyValues);
		TArray<ETargetPlatform> AddPlatforms = ParserPatchPlatforms(Params);
	
		if(AddPlatforms.Num())
		{
			for(auto& Platform:AddPlatforms)
			{
				ExportPatchSetting->PakTargetPlatforms.AddUnique(Platform);
			}
		}
		ExportPatchSetting->AssetIncludeFilters.Append(ParserPatchFilters(Params,TEXT("AssetIncludeFilters")));
		ExportPatchSetting->AssetIgnoreFilters.Append(ParserPatchFilters(Params,TEXT("AssetIgnoreFilters")));
		
		FString FinalConfig;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*ExportPatchSetting,FinalConfig);
		UE_LOG(LogHotPatcherCommandlet, Display, TEXT("%s"), *FinalConfig);

		
		UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->AddToRoot();
		PatcherProxy->SetProxySettings(ExportPatchSetting.Get());
		PatcherProxy->OnPaking.AddStatic(&::NSPatch::ReceiveMsg);
		PatcherProxy->OnShowMsg.AddStatic(&::NSPatch::ReceiveShowMsg);
		bExportStatus = PatcherProxy->DoExport();
		
		UE_LOG(LogHotPatcherCommandlet,Display,TEXT("Export Patch Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	}

	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return (int32)!bExportStatus;
}
