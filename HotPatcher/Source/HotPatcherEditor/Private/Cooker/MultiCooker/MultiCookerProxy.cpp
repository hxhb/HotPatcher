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
	CookPatchSettings.SavePath = GetSettingObject()->SavePath;
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

template<typename T>
TArray<TArray<T>> SplitArray(const TArray<T>& Array,int32 SplitNum)
{
	TArray<TArray<T>> result;
	result.AddDefaulted(SplitNum);

	for( int32 index=0; index<Array.Num(); ) 
	{
		for(auto& SplitItem:result)
		{
			SplitItem.Add(Array[index]);
			++index;
			if(index >= Array.Num())
			{
				break;
			}
		}
	}
	return result;
}

TArray<FSingleCookerSettings> UMultiCookerProxy::MakeSingleCookerSettings(const TArray<FAssetDetail>& AllDetails)
{
	int32 ProcessNumber = GetSettingObject()->ProcessNumber;
	
	TArray<FSingleCookerSettings> AllSingleCookerSettings;

	TMap<FString,TArray<FAssetDetail>> TypeAssetDetails;

	for(int32 index = 0;index<ProcessNumber;++index)
	{
		FSingleCookerSettings EmptySetting;
		EmptySetting.MissionID = index;
		EmptySetting.MissionName = FString::Printf(TEXT("SingleCooker_Part_%d"),EmptySetting.MissionID);
		EmptySetting.MultiCookerSettings = *GetSettingObject();
		AllSingleCookerSettings.Add(EmptySetting);
	}
	
	for(const auto& AssetDetail:AllDetails)
	{
		TArray<FAssetDetail>& Assets = TypeAssetDetails.FindOrAdd(AssetDetail.mAssetType);
		Assets.AddUnique(AssetDetail);
	}
	for(auto& TypeAssets:TypeAssetDetails)
	{
		TArray<TArray<FAssetDetail>> SplitInfo = SplitArray(TypeAssets.Value,ProcessNumber);
		for(int32 index = 0;index < ProcessNumber;++index)
		{
			AllSingleCookerSettings[index].CookAssets.Append(SplitInfo[index]);
		}
	}
	
	return AllSingleCookerSettings;
}

bool UMultiCookerProxy::DoExport()
{
	CookerProcessMap.Empty();
	CookerConfigMap.Empty();
	CookerFailedCollectionMap.Empty();
	ScanedCaches.Empty();
	
	FString TempDir = UFlibMultiCookerHelper::GetMultiCookerBaseDir();
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
	OnMultiCookerBegining.Broadcast(this);
	
	TArray<FSingleCookerSettings> SingleCookerSettings = MakeSingleCookerSettings(OutAssetDetails);

	for(const auto& CookerSetting:SingleCookerSettings)
	{
		TSharedPtr<FThreadWorker> ProcWorker = CreateSingleCookWroker(CookerSetting);
		ProcWorker->Execute();
	}
	
	if(GetSettingObject()->IsSaveConfig())
	{
		FString SaveConfigDir = UFlibPatchParserHelper::ReplaceMark(GetSettingObject()->SavePath.Path);

		FString CurrentConfig;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetSettingObject(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(SaveConfigDir,TEXT("MultiCookerConfig.json"));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	}
	if(IsRunningCommandlet())
	{
		TSharedPtr<FThreadWorker> WaitThreadWorker = MakeShareable(new FThreadWorker(TEXT("SingleCooker_WaitCookComplete"),[this]()
		{
			while (IsRunning())
			{
				FPlatformProcess::Sleep(0.0f);
			}
		}));
		WaitThreadWorker->Execute();
		WaitThreadWorker->Join();
	}
	return !HasError();
}

void UMultiCookerProxy::OnOutputMsg(FProcWorkerThread* Worker,const FString& InMsg)
{
	UE_LOG(LogHotPatcher,Display,TEXT("%s: %s"),*Worker->GetThreadName(), *InMsg);
}

void UMultiCookerProxy::OnCookProcBegin(FProcWorkerThread* ProcWorker)
{
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker %s Begining"),*ProcWorker->GetThreadName());
}

void UMultiCookerProxy::OnCookProcSuccessed(FProcWorkerThread* ProcWorker)
{
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker Proc %s Successed!"),*ProcWorker->GetThreadName());
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
		}
	}
	CookerFailedCollectionMap.FindOrAdd(ProcWorker->GetThreadName(),FailedMissionCollection);
	// CookerProcessMap.Remove(CookerProcName);
	UpdateSingleCookerStatus(false,FailedMissionCollection);
	UpdateMultiCookerStatus();
}

TSharedPtr<FProcWorkerThread> UMultiCookerProxy::CreateProcMissionThread(const FString& Bin,
                                                                         const FString& Command, const FString& MissionName)
{
	TSharedPtr<FProcWorkerThread> ProcWorkingThread;
	ProcWorkingThread = MakeShareable(new FProcWorkerThread(*MissionName, Bin, Command));
	ProcWorkingThread->ProcOutputMsgDelegate.BindUObject(this,&UMultiCookerProxy::OnOutputMsg);
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
