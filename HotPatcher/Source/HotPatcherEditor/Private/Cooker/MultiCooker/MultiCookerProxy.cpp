#include "Cooker/MultiCooker/MultiCookerProxy.h"

#include "FlibHotPatcherEditorHelper.h"
#include "HotPatcherEditor.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"

void UMultiCookerProxy::Init()
{
	Super::Init();
}

void UMultiCookerProxy::Shutdown()
{
	Super::Shutdown();
}

FExportPatchSettings UMultiCookerProxy::MakePatchSettings()
{
	FExportPatchSettings CookPatchSettings;
	CookPatchSettings.AssetIncludeFilters = GetSettingObject()->AssetIncludeFilters;
	CookPatchSettings.AssetIgnoreFilters = GetSettingObject()->AssetIgnoreFilters;
	CookPatchSettings.IncludeSpecifyAssets = GetSettingObject()->IncludeSpecifyAssets;
	CookPatchSettings.bIncludeHasRefAssetsOnly = GetSettingObject()->bIncludeHasRefAssetsOnly;
	CookPatchSettings.bAnalysisFilterDependencies = GetSettingObject()->bAnalysisFilterDependencies;
	CookPatchSettings.bForceSkipContent = GetSettingObject()->bForceSkipContent;
	CookPatchSettings.ForceSkipContentRules = GetSettingObject()->ForceSkipContentRules;
	CookPatchSettings.AssetRegistryDependencyTypes = GetSettingObject()->AssetRegistryDependencyTypes;
	return CookPatchSettings;
}

bool UMultiCookerProxy::DoExport()
{
	FString TempDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher/MultiCooker")));
	if(FPaths::DirectoryExists(TempDir))
	{
		IFileManager::Get().DeleteDirectory(*TempDir);
	}
	
	FExportPatchSettings CookPatchSettings = MakePatchSettings();
	FChunkInfo CookChunk = UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(&CookPatchSettings);
	
	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT("MultiCooker"),
		TEXT(""),
		FDateTime::UtcNow().ToString(),
		CookChunk,
		GetAssetsDependenciesScanedCaches(),
		CookPatchSettings.IsIncludeHasRefAssetsOnly(),
		CookPatchSettings.IsAnalysisFilterDependencies()
	);
	TArray<FAssetDetail> OutAssetDetails;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(CurrentVersion.AssetInfo,OutAssetDetails);

	int32 SingleProcessAssetNum = OutAssetDetails.Num() / GetSettingObject()->ProcessNumber;
	int32 RemainingAssetNum = 0;
	
	while(RemainingAssetNum < OutAssetDetails.Num())
	{
		int32 OldRemainingAssetNum = RemainingAssetNum;
		FSingleCookerSettings SingleCookerSettings;
		
		for(int32 index = RemainingAssetNum;index < OutAssetDetails.Num();++index)
		{
			if((RemainingAssetNum - OldRemainingAssetNum) > SingleProcessAssetNum)
			{
				break;
			}
			SingleCookerSettings.CookAssets.Add(OutAssetDetails[index]);
			++RemainingAssetNum;
		}
		SingleCookerSettings.MissionName = FString::Printf(TEXT("%d_%d"),OldRemainingAssetNum,RemainingAssetNum);
		SingleCookerSettings.MissionID = RemainingAssetNum - OldRemainingAssetNum;
		TSharedPtr<FThreadWorker> ProcWorker = CreateSingleCookWroker(SingleCookerSettings);
		ProcWorker->Execute();
	}
	
	return Super::DoExport();
}

void UMultiCookerProxy::OnOutputMsg(const FString& InMsg)
{
	UE_LOG(LogHotPatcher,Log,TEXT("%s"),*InMsg);
}

void UMultiCookerProxy::OnCookProcBegin(FProcWorkerThread* ProcWorker)
{
	UE_LOG(LogHotPatcher,Log,TEXT("Single Cooker %s Begining"),*ProcWorker->GetThreadName());
}

void UMultiCookerProxy::OnCookProcSuccessed(FProcWorkerThread* ProcWorker)
{
	UE_LOG(LogHotPatcher,Log,TEXT("Single Cooker Proc %s Successed!"),*ProcWorker->GetThreadName());
}

void UMultiCookerProxy::OnCookProcFailed(FProcWorkerThread* ProcWorker)
{
	UE_LOG(LogHotPatcher,Log,TEXT("Single Cooker Proc %s Failure!"),*ProcWorker->GetThreadName());
}

TSharedPtr<FProcWorkerThread> UMultiCookerProxy::CreateProcMissionThread(const FString& Bin,
                                                                         const FString& Command, const FString& MissionName)
{
	TSharedPtr<FProcWorkerThread> ProcWorkingThread;
	ProcWorkingThread = MakeShareable(new FProcWorkerThread(*MissionName, Bin, Command));
	ProcWorkingThread->ProcOutputMsgDelegate.AddUObject(this,&UMultiCookerProxy::OnOutputMsg);
	ProcWorkingThread->ProcBeginDelegate.AddUObject(this,&UMultiCookerProxy::OnCookProcBegin);
	ProcWorkingThread->ProcSuccessedDelegate.AddUObject(this,&UMultiCookerProxy::OnCookProcSuccessed);
	ProcWorkingThread->ProcFaildDelegate.AddUObject(this,&UMultiCookerProxy::OnCookProcFailed);
	return ProcWorkingThread;
}

TSharedPtr<FProcWorkerThread> UMultiCookerProxy::CreateSingleCookWroker(const FSingleCookerSettings& SingleCookerSettings)
{
	FString CurrentConfig;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(SingleCookerSettings,CurrentConfig);
	FString SaveConfigTo = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("HotPatcher/MultiCooker"),
			FString::Printf(TEXT("%s_Cooker_%d.json"),*SingleCookerSettings.MissionName,SingleCookerSettings.MissionID)
			));
	FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotSingleCooker -config=\"%s\" %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*GetSettingObject()->GetCombinedAdditionalCommandletArgs());
	UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher Cook Mission: %s %s"),*UFlibHotPatcherEditorHelper::GetUECmdBinary(),*MissionCommand);
	auto CookerProcThread = CreateProcMissionThread(UFlibHotPatcherEditorHelper::GetUECmdBinary(),MissionCommand,SingleCookerSettings.MissionName);
	CookerConfigMap.Add(SingleCookerSettings.MissionName,SingleCookerSettings);
	CookerProcessMap.Add(SingleCookerSettings.MissionName,CookerProcThread);
	return CookerProcThread;
}
