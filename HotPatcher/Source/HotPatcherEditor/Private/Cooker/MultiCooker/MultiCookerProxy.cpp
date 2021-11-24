#include "Cooker/MultiCooker/MultiCookerProxy.h"

#include "FlibHotPatcherEditorHelper.h"
#include "FlibMultiCookerHelper.h"
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

bool UMultiCookerProxy::IsRunning() const
{
	bool bHasValieWorker = false;
	for(auto& CookWorker:CookerProcessMap)
	{
		switch (CookWorker.Value->GetThreadStatus())
		{
		case EThreadStatus::InActive:
		case EThreadStatus::Busy:
			{
				bHasValieWorker = true;
				break;
			}
		}
		if(bHasValieWorker)
		{
			break;
		}
	}
	return bHasValieWorker;
}

void UMultiCookerProxy::Cancel()
{
	for(auto Cooker:CookerProcessMap)
	{
		Cooker.Value->Cancel();
	}
	// CookerProcessMap.Empty();
	// CookerConfigMap.Empty();
	// CookerFailedCollectionMap.Empty();
	// ScanedCaches.Empty();
}

bool UMultiCookerProxy::HasError()
{
	return !!CookerFailedCollectionMap.Num();
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

void UMultiCookerProxy::UpdateMultiCookerStatus()
{
	++FinishedCount;
	if(FinishedCount == CookerProcessMap.Num())
	{
		OnMultiCookerFinished.Broadcast(this);
	}
}

void UMultiCookerProxy::UpdateSingleCookerStatus(bool bSuccessed, const FAssetsCollection& FailedCollection)
{
	
}
struct FSplitArrayIndex
{
	int32 BeginPos;
	int32 EndPos;
};

template<typename T>
TArray<FSplitArrayIndex> SplitArray(const TArray<T>& Array,int32 SplitNum)
{
	TArray<FSplitArrayIndex> result;
	int32 Number = Array.Num() / SplitNum;
	for(int32 index = 0;index < SplitNum;++index)
	{
		FSplitArrayIndex Item;
		Item.BeginPos = (index * Number) > (Array.Num() -1) ? (index * Number) : Array.Num() -1;
		Item.BeginPos = (index * Number + Number -1) > (Array.Num() -1) ? (index * Number + Number -1) : Array.Num() -1;
		result.Add(Item);
	}
	return result;
}

TArray<FSingleCookerSettings> UMultiCookerProxy::MakeSingleCookerSettings(const TArray<FAssetDetail>& AllDetails)
{
	int32 ProcessNumber = GetSettingObject()->ProcessNumber;
	
	TArray<FSingleCookerSettings> AllSingleCookerSettings;

	TMap<FString,TArray<FAssetDetail>> TypeAssetDetails;

	AllSingleCookerSettings.AddDefaulted(ProcessNumber);
	
	for(const auto& AssetDetail:AllDetails)
	{
		TArray<FAssetDetail>& Assets = TypeAssetDetails.FindOrAdd(AssetDetail.mAssetType);
		Assets.AddUnique(AssetDetail);
	}
	for(auto& TypeAssets:TypeAssetDetails)
	{
		TArray<FSplitArrayIndex> SplitInfo = SplitArray(TypeAssets.Value,ProcessNumber);
		for(int32 index = 0;index < ProcessNumber;++index)
		{
			for(int32 Pos = SplitInfo[index].BeginPos;Pos <= SplitInfo[index].EndPos; ++Pos)
			{
				AllSingleCookerSettings[index].CookAssets.Add(TypeAssets.Value[Pos]);
			}
		}
	}
	
	return AllSingleCookerSettings;
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

	OnMultiCookerBegining.Broadcast(this);
	
	while(RemainingAssetNum < OutAssetDetails.Num())
	{
		int32 OldRemainingAssetNum = RemainingAssetNum;
		FSingleCookerSettings SingleCookerSettings;
		SingleCookerSettings.MultiCookerSettings = *GetSettingObject();
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
	
	return !HasError();
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
	FString CookerProcName = ProcWorker->GetThreadName();
	UpdateSingleCookerStatus(true,FAssetsCollection{});
	UpdateMultiCookerStatus();
}

void UMultiCookerProxy::OnCookProcFailed(FProcWorkerThread* ProcWorker)
{
	FString CookerProcName = ProcWorker->GetThreadName();
	UE_LOG(LogHotPatcher,Log,TEXT("Single Cooker Proc %s Failure!"),*CookerProcName);
	FSingleCookerSettings* CookerSettings =  CookerConfigMap.Find(CookerProcName);
	FString CookFailedResultPath = UFlibMultiCookerHelper::GetCookerProcFailedResultPath(CookerSettings->MissionName,CookerSettings->MissionID);
	FAssetsCollection FailedMissionCollection;
	if(FPaths::FileExists(CookFailedResultPath))
	{
		FString FailedContent;
		if(FFileHelper::LoadFileToString(FailedContent,*CookFailedResultPath))
		{
			UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(FailedContent,FailedMissionCollection);
			CookerFailedCollectionMap.FindOrAdd(ProcWorker->GetThreadName(),FailedMissionCollection);
		}
	}
	// CookerProcessMap.Remove(CookerProcName);
	UpdateSingleCookerStatus(false,FailedMissionCollection);
	UpdateMultiCookerStatus();
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
	FString SaveConfigTo = UFlibMultiCookerHelper::GetCookerProcConfigPath(SingleCookerSettings.MissionName,SingleCookerSettings.MissionID);
	
	FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotSingleCooker -config=\"%s\" -DDCNOSAVEBOOT -NoAssetRegistryCache %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*GetSettingObject()->GetCombinedAdditionalCommandletArgs());
	UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher Cook Mission: %s %s"),*UFlibHotPatcherEditorHelper::GetUECmdBinary(),*MissionCommand);
	auto CookerProcThread = CreateProcMissionThread(UFlibHotPatcherEditorHelper::GetUECmdBinary(),MissionCommand,SingleCookerSettings.MissionName);
	CookerConfigMap.Add(SingleCookerSettings.MissionName,SingleCookerSettings);
	CookerProcessMap.Add(SingleCookerSettings.MissionName,CookerProcThread);
	return CookerProcThread;
}
