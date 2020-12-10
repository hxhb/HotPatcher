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
		UE_LOG(LogHotPatcherCommandlet,Log,TEXT("%s"),*InMsg);
	}
}

int32 UHotPatcherCommandlet::Main(const FString& Params)
{
	UE_LOG(LogHotPatcherCommandlet, Display, TEXT("UHotPatcherCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotPatcherCommandlet, Error, TEXT("UHotPatcherCommandlet error: not -config=xxxx.json params."));
		return -1;
	}

	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotPatcherCommandlet, Error, TEXT("UHotPatcherCommandlet error: cofnig file %s not exists."), *config_path);
		return -1;
	}

	FString JsonContent;
	bool bExportStatus = false;
	if (FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		TSharedPtr<FExportPatchSettings> ExportPatchSetting = MakeShareable(new FExportPatchSettings);
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportPatchSetting);
		UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->SetProxySettings(ExportPatchSetting.Get());
		PatcherProxy->OnPaking.AddStatic(&::NSPatch::ReceiveMsg);
		PatcherProxy->OnShowMsg.AddStatic(&::NSPatch::ReceiveShowMsg);
		bExportStatus = PatcherProxy->DoExport();
		
		UE_LOG(LogHotPatcherCommandlet,Log,TEXT("Export Patch Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	}
	system("pause");
	return (int32)!bExportStatus;
}
