#include "Cooker/MultiCooker/MultiCookerProxy.h"

#include "FlibHotPatcherEditorHelper.h"
#include "FlibMultiCookerHelper.h"
#include "HotPatcherEditor.h"
#include "Cooker/MultiCooker/MultiCookScheduler.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "Async/Async.h"

void UMultiCookerProxy::Init()
{
	RecookerProxy = NewObject<USingleCookerProxy>();
	RecookerProxy->AddToRoot();
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

void UMultiCookerProxy::OnCookMissionsFinished(bool bSuccessed)
{
	if(bSuccessed && GetSettingObject()->ShaderOptions.bMergeShaderLibrary)
	{
		MergeShader();
	}
	if(!bSuccessed)
	{
		RecookFailedAssets();
	}
	OnMultiCookerFinished.Broadcast(this);
}

bool UMultiCookerProxy::MergeShader()
{
	FString BaseDir = UFlibMultiCookerHelper::GetMultiCookerBaseDir();
	TArray<FString> PlatformNames;
	TArray<FShaderCodeFormatMap> ShaderCodeFormatMaps;
	
	for(const auto& Platform:GetSettingObject()->CookTargetPlatforms)
	{
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
		ITargetPlatform* TargetPlatform = UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName);
		PlatformNames.AddUnique(PlatformName);
		FShaderCodeFormatMap ShaderCodeFormatMap;
		ShaderCodeFormatMap.Platform = TargetPlatform;
		ShaderCodeFormatMap.bIsNative = true;
		ShaderCodeFormatMap.SaveBaseDir = FPaths::Combine(BaseDir,PlatformName);
		for(const auto& Cooker:CookerConfigMap)
		{
			
			FString CurrentCookerDir = FPaths::Combine(BaseDir,Cooker.Key,PlatformName,TEXT("Metadata/ShaderLibrarySource"));
			
			TArray<FName> ShaderFormats;
			TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);
			for(auto& ShaderFormat:ShaderFormats)
			{
				TArray<FString > FoundShaderFiles = UFlibShaderCodeLibraryHelper::FindCookedShaderLibByShaderFrmat(ShaderFormat.ToString(),CurrentCookerDir);

				if(FoundShaderFiles.Num())
				{
					FString FormatExtersion = FString::Printf(TEXT("-%s.ushaderbytecode"),*ShaderFormat.ToString());
					
					FString ShderName = FoundShaderFiles[0];
					ShderName.RemoveFromStart(TEXT("ShaderArchive-"));
					ShderName.RemoveFromEnd(FormatExtersion);
					FShaderCodeFormatMap::FShaderFormatNameFiles ShaderNameAndFileMap;
					ShaderNameAndFileMap.ShaderName = ShderName;
					ShaderNameAndFileMap.Files = FoundShaderFiles;
					ShaderCodeFormatMap.ShaderCodeTypeFilesMap.Add(ShaderFormat.ToString(),ShaderNameAndFileMap);
				}
			}
		}
		ShaderCodeFormatMaps.Add(ShaderCodeFormatMap);
	}
	
	TSharedPtr<FMergeShaderCollectionProxy> MergeShaderCollectionProxy = MakeShareable(new FMergeShaderCollectionProxy(ShaderCodeFormatMaps));
	return true;
}

void UMultiCookerProxy::RecookFailedAssets()
{
	TMap<ETargetPlatform,TArray<FAssetDetail>> AllFailedAssetsMap;
	FSingleCookerSettings SingleCookerSetting;
	SingleCookerSetting.MultiCookerSettings.ShaderOptions.bSharedShaderLibrary = false;
	TArray<FAssetDetail>& AllFailedAssets = SingleCookerSetting.CookAssets;
	for(const auto& CookFailedCollection:CookerFailedCollectionMap)
	{
		AllFailedAssetsMap.FindOrAdd(CookFailedCollection.Value.TargetPlatform).Append(CookFailedCollection.Value.Assets);
		for(const auto& FailedAsset:CookFailedCollection.Value.Assets)
		{
			AllFailedAssets.AddUnique(FailedAsset);
		}
	}
	
	AllFailedAssetsMap.GetKeys(SingleCookerSetting.MultiCookerSettings.CookTargetPlatforms);

	if(RecookerProxy)
	{
		RecookerProxy->SetProxySettings(&SingleCookerSetting);
		RecookerProxy->DoExport();
	}
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
	FScopeLock Lock(&SynchronizationObject);
	++FinishedCount;
	if(FinishedCount == CookerProcessMap.Num())
	{
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			OnCookMissionsFinished(!HasError());
		});
	}
}

void UMultiCookerProxy::UpdateSingleCookerStatus(FProcWorkerThread* ProcWorker, bool bSuccessed, const FAssetsCollection& FailedCollection)
{
	if(!bSuccessed)
	{
		CookerFailedCollectionMap.FindOrAdd(ProcWorker->GetThreadName(),FailedCollection);
	}
}


TArray<FSingleCookerSettings> UMultiCookerProxy::MakeSingleCookerSettings(const TArray<FAssetDetail>& AllDetails)
{
	UMultiCookScheduler* DefaultScheduler = Cast<UMultiCookScheduler>(GetSettingObject()->Scheduler->GetDefaultObject());
	return DefaultScheduler->MultiCookScheduler(*GetSettingObject(),AllDetails);
}

bool UMultiCookerProxy::DoExport()
{
	CookerProcessMap.Empty();
	CookerConfigMap.Empty();
	CookerFailedCollectionMap.Empty();
	ScanedCaches.Empty();
	
	FString TempDir = UFlibMultiCookerHelper::GetMultiCookerBaseDir();
	FString SaveConfigDir = UFlibPatchParserHelper::ReplaceMark(GetSettingObject()->SavePath.Path);
	
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
	FMultiCookerAssets MultiCookerAssets;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(CurrentVersion.AssetInfo,MultiCookerAssets.Assets);
	{
		FString SaveNeedCookAssets;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(MultiCookerAssets,SaveNeedCookAssets);
		FString SaveMultiCookerAssetsTo= FPaths::ConvertRelativePathToFull(SaveConfigDir,TEXT("MultiCookerAssets.json"));
		FFileHelper::SaveStringToFile(SaveNeedCookAssets,*SaveMultiCookerAssetsTo);
	}
	OnMultiCookerBegining.Broadcast(this);
	
	TArray<FSingleCookerSettings> SingleCookerSettings = MakeSingleCookerSettings(MultiCookerAssets.Assets);

	for(const auto& CookerSetting:SingleCookerSettings)
	{
		TSharedPtr<FThreadWorker> ProcWorker = CreateSingleCookWroker(CookerSetting);
		ProcWorker->Execute();
	}
	
	if(GetSettingObject()->IsSaveConfig())
	{
		FString CurrentConfig;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetSettingObject(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(SaveConfigDir,FString::Printf(TEXT("%s_MultiCookerConfig.json"),FApp::GetProjectName()));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	}
	if(IsRunningCommandlet())
	{
		TSharedPtr<FThreadWorker> WaitThreadWorker = MakeShareable(new FThreadWorker(TEXT("SingleCooker_WaitCookComplete"),[this]()
		{
			while (IsRunning())
			{
				FPlatformProcess::Sleep(1.f);
			}
		}));
		WaitThreadWorker->Execute();
		WaitThreadWorker->Join();
	}
	return !HasError();
}

void UMultiCookerProxy::OnOutputMsg(FProcWorkerThread* Worker,const FString& InMsg)
{
	FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("%s: %s"),*Worker->GetThreadName(), *InMsg);
}

void UMultiCookerProxy::OnCookProcBegin(FProcWorkerThread* ProcWorker)
{
	FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker %s Begining"),*ProcWorker->GetThreadName());
}

void UMultiCookerProxy::OnCookProcSuccessed(FProcWorkerThread* ProcWorker)
{
	FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker Proc %s Successed!"),*ProcWorker->GetThreadName());
	FString CookerProcName = ProcWorker->GetThreadName();
	UpdateSingleCookerStatus(ProcWorker,true, FAssetsCollection{});
	UpdateMultiCookerStatus();
}

void UMultiCookerProxy::OnCookProcFailed(FProcWorkerThread* ProcWorker)
{
	FScopeLock Lock(&SynchronizationObject);
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
	// CookerProcessMap.Remove(CookerProcName);
	UpdateSingleCookerStatus(ProcWorker,false, FailedMissionCollection);
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
	FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotSingleCooker -config=\"%s\" -DDCNOSAVEBOOT -NoAssetRegistryCache -stdout -CrashForUAT -unattended -NoLogTimes -UTF8Output %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*GetSettingObject()->GetCombinedAdditionalCommandletArgs());
	UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher Cook Mission: %s %s"),*UFlibHotPatcherEditorHelper::GetUECmdBinary(),*MissionCommand);
	auto CookerProcThread = CreateProcMissionThread(UFlibHotPatcherEditorHelper::GetUECmdBinary(),MissionCommand,SingleCookerSettings.MissionName);
	CookerConfigMap.Add(SingleCookerSettings.MissionName,SingleCookerSettings);
	CookerProcessMap.Add(SingleCookerSettings.MissionName,CookerProcThread);
	return CookerProcThread;
}
