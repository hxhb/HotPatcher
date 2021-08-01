#include "HotReleaseCommandlet.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "CreatePatch/ReleaseProxy.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotReleaseCommandlet);

#define PATCHER_CONFIG_PARAM_NAME TEXT("-config=")

namespace NSRelease
{
	void ReceiveMsg(const FString& InMsgType,const FString& InMsg)
	{
		UE_LOG(LogHotReleaseCommandlet,Log,TEXT("%s:%s"),*InMsgType,*InMsg);
	}

	void ReceiveShowMsg(const FString& InMsg)
	{
		UE_LOG(LogHotReleaseCommandlet,Log,TEXT("%s"),*InMsg);
	}
}

#define ADD_PLATFORM_PAK_LIST TEXT("AddPlatformPakList")
TArray<FPlatformPakListFiles> ParserPlatformPakList(const FString& Commandline)
{
	TArray<FPlatformPakListFiles> result;
	TMap<FString, FString> KeyValues = UFlibPatchParserHelper::GetCommandLineParamsMap(Commandline);
	if(KeyValues.Find(ADD_PLATFORM_PAK_LIST))
	{
		FString AddPakListInfo = *KeyValues.Find(ADD_PLATFORM_PAK_LIST);
		TArray<FString> PlatformPakLists;
		AddPakListInfo.ParseIntoArray(PlatformPakLists,TEXT(","));

		for(auto& PlatformPakList:PlatformPakLists)
		{
			TArray<FString> PlatformPakListToken;
			PlatformPakList.ParseIntoArray(PlatformPakListToken,TEXT("+"));
			if(PlatformPakListToken.Num() >= 2)
			{
				FString PlatformName = PlatformPakListToken[0];
				FPlatformPakListFiles PlatformPakListItem;
				UFlibPatchParserHelper::GetEnumValueByName(PlatformName,PlatformPakListItem.TargetPlatform);
				for(int32 index=1;index<PlatformPakListToken.Num();++index)
				{
					FString PakListPath = PlatformPakListToken[index];
					if(FPaths::FileExists(PakListPath))
					{
						FFilePath PakFilePath;
						PakFilePath.FilePath = PakListPath;
						PlatformPakListItem.PakResponseFiles.Add(PakFilePath);
					}
				}
				result.Add(PlatformPakListItem);
			}
		}
	}
	return result;
}

int32 UHotReleaseCommandlet::Main(const FString& Params)
{
	UE_LOG(LogHotReleaseCommandlet, Display, TEXT("UHotReleaseCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotReleaseCommandlet, Warning, TEXT("not -config=xxxx.json params."));
		// return -1;
	}

	if (bStatus && !FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotReleaseCommandlet, Error, TEXT("cofnig file %s not exists."), *config_path);
		return -1;
	}
	if(IsRunningCommandlet())
	{
		// load asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().SearchAllAssets(true);
	}

	TSharedPtr<FExportReleaseSettings> ExportReleaseSetting = MakeShareable(new FExportReleaseSettings);
	
	FString JsonContent;
	if (FPaths::FileExists(config_path) && FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportReleaseSetting);
	}

	TMap<FString, FString> KeyValues = UFlibPatchParserHelper::GetCommandLineParamsMap(Params);
	UFlibPatchParserHelper::ReplaceProperty(*ExportReleaseSetting, KeyValues);

	// 从命令行分析PlatformPakList
	TArray<FPlatformPakListFiles> ReadPakList = ParserPlatformPakList(Params);
	if(ReadPakList.Num())
	{
		ExportReleaseSetting->PlatformsPakListFiles = ReadPakList;
	}

	if(ReadPakList.Num() && ExportReleaseSetting->IsBackupMetadata())
	{
		for(const auto& PlatformPakList:ReadPakList)
		{
			ExportReleaseSetting->BackupMetadataPlatforms.AddUnique(PlatformPakList.TargetPlatform);
        }
	}
	
	FString FinalConfig;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(*ExportReleaseSetting,FinalConfig);
	UE_LOG(LogHotReleaseCommandlet, Display, TEXT("%s"), *FinalConfig);
		
	UReleaseProxy* ReleaseProxy = NewObject<UReleaseProxy>();
	ReleaseProxy->AddToRoot();
	ReleaseProxy->SetProxySettings(ExportReleaseSetting.Get());
	ReleaseProxy->OnPaking.AddStatic(&::NSRelease::ReceiveMsg);
	ReleaseProxy->OnShowMsg.AddStatic(&::NSRelease::ReceiveShowMsg);
	bool bExportStatus = ReleaseProxy->DoExport();
		
	UE_LOG(LogHotReleaseCommandlet,Display,TEXT("Export Release Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return (int32)!bExportStatus;
}
