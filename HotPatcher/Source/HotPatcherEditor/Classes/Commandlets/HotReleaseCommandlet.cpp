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
		UE_LOG(LogHotReleaseCommandlet,Display,TEXT("%s:%s"),*InMsgType,*InMsg);
	}

	void ReceiveShowMsg(const FString& InMsg)
	{
		UE_LOG(LogHotReleaseCommandlet,Log,TEXT("%s"),*InMsg);
	}
}


int32 UHotReleaseCommandlet::Main(const FString& Params)
{
	UE_LOG(LogHotReleaseCommandlet, Display, TEXT("UHotReleaseCommandlet::Main"));

	FString config_path;
	bool bStatus = FParse::Value(*Params, *FString(PATCHER_CONFIG_PARAM_NAME).ToLower(), config_path);
	if (!bStatus)
	{
		UE_LOG(LogHotReleaseCommandlet, Error, TEXT("UHotReleaseCommandlet error: not -config=xxxx.json params."));
		return -1;
	}

	if (!FPaths::FileExists(config_path))
	{
		UE_LOG(LogHotReleaseCommandlet, Error, TEXT("UHotReleaseCommandlet error: cofnig file %s not exists."), *config_path);
		return -1;
	}

	FString JsonContent;
	bool bExportStatus = false;
	if (FFileHelper::LoadFileToString(JsonContent, *config_path))
	{
		TSharedPtr<FExportReleaseSettings> ExportReleaseSetting = MakeShareable(new FExportReleaseSettings);
		UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(JsonContent,*ExportReleaseSetting);
		UReleaseProxy* ReleaseProxy = NewObject<UReleaseProxy>();
		ReleaseProxy->SetProxySettings(ExportReleaseSetting.Get());
		ReleaseProxy->OnPaking.AddStatic(&::NSRelease::ReceiveMsg);
		ReleaseProxy->OnShowMsg.AddStatic(&::NSRelease::ReceiveShowMsg);
		bExportStatus = ReleaseProxy->DoExport();
		
		UE_LOG(LogHotReleaseCommandlet,Log,TEXT("Export Release Misstion is %s!"),bExportStatus?TEXT("Successed"):TEXT("Failure"));
	}
	system("pause");
	return (int32)!bExportStatus;
}
