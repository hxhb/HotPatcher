#include "HotPatcherCommandlet.h"
// #include "CreatePatch/ExportPatchSettings.h"
#include "CreatePatch/PatcherProxy.h"

// engine header
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogHotPatcherCommandlet);

#define PATCHER_CONFIG_PARAM_NAME TEXT("-config=")

void ReceiveMsg(const FString& InMsgType,const FString& InMsg)
{
	UE_LOG(LogHotPatcherCommandlet,Display,TEXT("%s:%s"),*InMsgType,*InMsg);
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
	if (FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		class UExportPatchSettings* ExportPatchSetting = NewObject<class UExportPatchSettings>();
		UFlibHotPatcherEditorHelper::DeserializePatchConfig(ExportPatchSetting, JsonContent);
		UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->SetExportPatchSetting(ExportPatchSetting);
		PatcherProxy->OnPaking.AddStatic(&::ReceiveMsg);
		PatcherProxy->DoExportPatch();
	}
	system("pause");
	return 0;
}
