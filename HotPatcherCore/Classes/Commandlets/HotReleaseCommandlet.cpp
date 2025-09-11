#include "HotReleaseCommandlet.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "CreatePatch/ReleaseProxy.h"
#include "CommandletHelper.h"
#include "FlibHotPatcherCoreHelper.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotReleaseCommandlet);


TArray<FPlatformPakListFiles> ParserPlatformFilesByCmd(const FString& Commandline,const FString& Key,TFunction<void(FPlatformPakListFiles&,const FFilePath& File)> FileCallback)
{
	TArray<FPlatformPakListFiles> result;
	TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Commandline);
	if(KeyValues.Find(Key))
	{
		FString AddPakListInfo = *KeyValues.Find(Key);
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
				THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,PlatformPakListItem.TargetPlatform);
				for(int32 index=1;index<PlatformPakListToken.Num();++index)
				{
					FString PakListPath = PlatformPakListToken[index];
					if(FPaths::FileExists(PakListPath))
					{
						FFilePath PakFilePath;
						PakFilePath.FilePath = PakListPath;
						FileCallback(PlatformPakListItem,PakFilePath);
					}
				}
				result.Add(PlatformPakListItem);
			}
		}
	}
	return result;
}

#define ADD_PLATFORM_PAK_LIST TEXT("AddPlatformPakList")
#define ADD_PLATFORM_PAK_FILES TEXT("AddPlatformPakFiles")
TArray<FPlatformPakListFiles> ParserPlatformPakList(const FString& Commandline)
{
	TArray<FPlatformPakListFiles> result;
	result.Append(ParserPlatformFilesByCmd(Commandline,ADD_PLATFORM_PAK_LIST,[](FPlatformPakListFiles& PlatformPakListItem,const FFilePath& File)
	{
		PlatformPakListItem.PakResponseFiles.Add(File);
	}));
	result.Append(ParserPlatformFilesByCmd(Commandline,ADD_PLATFORM_PAK_FILES,[](FPlatformPakListFiles& PlatformPakListItem,const FFilePath& File)
	{
		PlatformPakListItem.PakFiles.Add(File);
	}));
	return result;
}

int32 UHotReleaseCommandlet::Main(const FString& Params)
{
	Super::Main(Params);
	UE_LOG(LogHotReleaseCommandlet, Display, TEXT("UHotReleaseCommandlet::Main:%s"), *Params);

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotReleaseCommandlet, Warning, TEXT("not -config=xxxx.json params."));
		// return -1;
	}
	config_path = UFlibPatchParserHelper::ReplaceMark(config_path);
	if (bStatus && !FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotReleaseCommandlet, Error, TEXT("config file %s not exists."), *config_path);
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
		THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportReleaseSetting);
		// adaptor old version config
		UFlibHotPatcherCoreHelper::AdaptorOldVersionConfig(ExportReleaseSetting->GetAssetScanConfigRef(),JsonContent);
	}

	TMap<FString, FString> KeyValues = THotPatcherTemplateHelper::GetCommandLineParamsMap(Params);
	THotPatcherTemplateHelper::ReplaceProperty(*ExportReleaseSetting, KeyValues);

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
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(*ExportReleaseSetting,FinalConfig);
	UE_LOG(LogHotReleaseCommandlet, Display, TEXT("%s"), *FinalConfig);
		
	UReleaseProxy* ReleaseProxy = NewObject<UReleaseProxy>();
	ReleaseProxy->AddToRoot();
	ReleaseProxy->Init(ExportReleaseSetting.Get());
	ReleaseProxy->OnPaking.AddStatic(&::CommandletHelper::ReceiveMsg);
	ReleaseProxy->OnShowMsg.AddStatic(&::CommandletHelper::ReceiveShowMsg);
	bool bExportStatus = ReleaseProxy->DoExport();
		
	UE_LOG(LogHotReleaseCommandlet,Display,TEXT("Export Release Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	
	if(FParse::Param(FCommandLine::Get(), TEXT("wait")))
	{
		system("pause");
	}
	
	return (int32)!bExportStatus;
}
