#include "Cooker/MultiCooker/MultiCookerProxy.h"

#include "FlibHotPatcherEditorHelper.h"
#include "FlibMultiCookerHelper.h"
#include "HotPatcherEditor.h"
#include "Cooker/MultiCooker/MultiCookScheduler.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "Async/Async.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"

void UMultiCookerProxy::Init()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::Init"),FColor::Red);

	GlobalShaderCollectionProxy = UFlibMultiCookerHelper::CreateCookShaderCollectionProxyByPlatform(
		FApp::GetProjectName(),
		GetSettingObject()->CookTargetPlatforms,
		GetSettingObject()->ShaderOptions.bSharedShaderLibrary,
		GetSettingObject()->ShaderOptions.bNativeShader,
		true,
		FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),TEXT("Shaders"))
	);
	if(GlobalShaderCollectionProxy.IsValid())
	{
		GlobalShaderCollectionProxy->Init();
	}

	if(GetSettingObject()->bImportProjectSettings)
	{
		GetSettingObject()->ImportProjectSettings();
	}
	Super::Init();
}

void UMultiCookerProxy::Shutdown()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::Shutdown"),FColor::Red);
	if(GlobalShaderCollectionProxy.IsValid())
	{
		GlobalShaderCollectionProxy->Shutdown();
	}
	Super::Shutdown();
}

bool UMultiCookerProxy::IsRunning() const
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::IsRunning"),FColor::Red);
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
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::Cancel"),FColor::Red);
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
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::HasError"),FColor::Red);
	return !!CookerFailedCollectionMap.Num();
}

void UMultiCookerProxy::OnCookMissionsFinished(bool bSuccessed)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::OnCookMissionsFinished"),FColor::Red);
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
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::MergeShader"),FColor::Red);
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
		ShaderCodeFormatMap.SaveBaseDir = FPaths::Combine(BaseDir,TEXT("Shaders"),PlatformName);
		for(const auto& Cooker:CookerConfigMap)
		{
			FString CurrentCookerDir = FPaths::Combine(BaseDir,Cooker.Key,PlatformName,TEXT("Metadata/ShaderLibrarySource"));
			
			TArray<FName> ShaderFormats;
			TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);

			TArray<FString> PlatformSCLCSVPaths;
			for(auto& ShaderFormat:ShaderFormats)
			{
				FString ShaderIntermediateLocation = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Shaders") / ShaderFormat.ToString());
				FString DefaultShaderLibIntermediateLocation = UFlibShaderPatchHelper::GetCodeArchiveFilename(ShaderIntermediateLocation,FApp::GetProjectName(),ShaderFormat);

				const FString RootMetaDataPath = ShaderIntermediateLocation / TEXT("Metadata") / TEXT("PipelineCaches");
				UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,TArray<FName>{ShaderFormat},ShaderIntermediateLocation,RootMetaDataPath,true);
				// load default shader,etc Saved/Shaders/PCD3D_SM5/ShaderArchive-Blank425-PCD3D_SM5.ushaderbytecode
				// FShaderCodeLibrary::SaveShaderCode(ShaderIntermediateLocation, RootMetaDataPath, ShaderFormats, PlatformSCLCSVPaths
				// #if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
				// ,NULL
				// #endif
				// );
				TArray<FString > FoundShaderFiles = UFlibShaderCodeLibraryHelper::FindCookedShaderLibByShaderFrmat(ShaderFormat.ToString(),CurrentCookerDir);
				for(const auto& ShaderArchiveFileName:FoundShaderFiles)
				{
					FString CopyTo = UFlibShaderPatchHelper::GetCodeArchiveFilename(ShaderCodeFormatMap.SaveBaseDir,FApp::GetProjectName(),ShaderFormat);
					IFileManager::Get().Copy(*CopyTo,*FPaths::Combine(CurrentCookerDir,ShaderArchiveFileName));
					IFileManager::Get().Copy(*DefaultShaderLibIntermediateLocation,*FPaths::Combine(CurrentCookerDir,ShaderArchiveFileName));
				}
			}
			FString ShaderCodeDir = ShaderCodeFormatMap.SaveBaseDir; //FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),Cooker.Value.MissionName,PlatformName);
			const FString RootMetaDataPath = ShaderCodeDir / TEXT("Metadata") / TEXT("PipelineCaches");

			UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,ShaderFormats,ShaderCodeDir,RootMetaDataPath,true);
			// // merge shader
			// FShaderCodeLibrary::SaveShaderCode(ShaderCodeDir, RootMetaDataPath, ShaderFormats, PlatformSCLCSVPaths
			// #if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
			// 	,NULL
			// #endif
			// );
		}
		ShaderCodeFormatMaps.Add(ShaderCodeFormatMap);
	}
	
	// TSharedPtr<FMergeShaderCollectionProxy> MergeShaderCollectionProxy = MakeShareable(new FMergeShaderCollectionProxy(ShaderCodeFormatMaps));
	return true;
}

void UMultiCookerProxy::RecookFailedAssets()
{
	FScopeLock Lock(&SynchronizationObject);
	RecookerProxy = NewObject<USingleCookerProxy>();
	RecookerProxy->AddToRoot();
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::RecookFailedAssets"),FColor::Red);
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
	// remove child process failed assets
	CookerFailedCollectionMap.Empty();
	
	// collection recook failed assets
	FCookerFailedCollection& CookerFailedCollection = RecookerProxy->GetCookFailedAssetsCollection();
	if(!!CookerFailedCollection.CookFailedAssets.Num())
	{
		TArray<ETargetPlatform> FailedPlatforms;
		CookerFailedCollection.CookFailedAssets.GetKeys(FailedPlatforms);
		for(auto TargetPlatform:FailedPlatforms)
		{
			FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(TargetPlatform);
			CookerFailedCollectionMap.FindOrAdd(PlatformName,*CookerFailedCollection.CookFailedAssets.Find(TargetPlatform));
			UE_LOG(LogHotPatcher,Error,TEXT("\nCook Platfotm %s Assets Failed:\n"),*PlatformName);
			for(const auto& Asset:CookerFailedCollection.CookFailedAssets.Find(TargetPlatform)->Assets)
			{
				UE_LOG(LogHotPatcher,Error,TEXT("\t%s\n"),*Asset.mPackagePath);
			}
		}
	}
}

FExportPatchSettings UMultiCookerProxy::MakePatchSettings()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::MakePatchSettings"),FColor::Red);
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
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::UpdateMultiCookerStatus"),FColor::Red);
	FScopeLock Lock(&SynchronizationObject);
	++FinishedCount;
	if(FinishedCount == CookerProcessMap.Num())
	{
		AsyncTask(ENamedThreads::GameThread,[this]()
		{
			OnCookMissionsFinished(!HasError());
		});
	}
}

void UMultiCookerProxy::UpdateSingleCookerStatus(FProcWorkerThread* ProcWorker, bool bSuccessed, const FAssetsCollection& FailedCollection)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::UpdateSingleCookerStatus"),FColor::Red);
	if(!bSuccessed)
	{
		CookerFailedCollectionMap.FindOrAdd(ProcWorker->GetThreadName(),FailedCollection);
	}
}


TArray<FSingleCookerSettings> UMultiCookerProxy::MakeSingleCookerSettings(const TArray<FAssetDetail>& AllDetails)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::MakeSingleCookerSettings"),FColor::Red);
	UMultiCookScheduler* DefaultScheduler = Cast<UMultiCookScheduler>(GetSettingObject()->Scheduler->GetDefaultObject());
	return DefaultScheduler->MultiCookScheduler(*GetSettingObject(),AllDetails);
}

bool UMultiCookerProxy::DoExport()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::DoExport"),FColor::Red);
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
	
	FHotPatcherVersion CurrentVersion;
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("ExportReleaseVersionInfoByChunk"),FColor::Red);
		CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		   TEXT("MultiCooker"),
		   TEXT(""),
		   FDateTime::UtcNow().ToString(),
		   CookChunk,
		   GetAssetsDependenciesScanedCaches(),
		   CookPatchSettings.IsIncludeHasRefAssetsOnly(),
		   CookPatchSettings.IsAnalysisFilterDependencies()
	   );
	}
	FMultiCookerAssets MultiCookerAssets;
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("GetAssetDetailsByAssetDependenciesInfo and Serialize to json"),FColor::Red);
		UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(CurrentVersion.AssetInfo,MultiCookerAssets.Assets);
		FString SaveNeedCookAssets;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(MultiCookerAssets,SaveNeedCookAssets);
		FString SaveMultiCookerAssetsTo= FPaths::ConvertRelativePathToFull(SaveConfigDir,FString::Printf(TEXT("%s_MultiCookerAssets.json"),FApp::GetProjectName()));
		FFileHelper::SaveStringToFile(SaveNeedCookAssets,*SaveMultiCookerAssetsTo);
	}
	OnMultiCookerBegining.Broadcast(this);
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("MakeSingleCookerSettings and Execute"),FColor::Red);
		TArray<FSingleCookerSettings> SingleCookerSettings = MakeSingleCookerSettings(MultiCookerAssets.Assets);

		for(const auto& CookerSetting:SingleCookerSettings)
		{
			TSharedPtr<FThreadWorker> ProcWorker = CreateSingleCookWroker(CookerSetting);
			ProcWorker->Execute();
		}
	}
	if(GetSettingObject()->IsSaveConfig())
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("Save Multi Cooker Config"),FColor::Red);
		FString CurrentConfig;
		UFlibPatchParserHelper::TSerializeStructAsJsonString(*GetSettingObject(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(SaveConfigDir,FString::Printf(TEXT("%s_MultiCookerConfig.json"),FApp::GetProjectName()));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	}
	return !HasError();
}

void UMultiCookerProxy::OnOutputMsg(FProcWorkerThread* Worker,const FString& InMsg)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::OnOutputMsg"),FColor::Red);
	// FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("%s: %s"),*Worker->GetThreadName(), *InMsg);
}

void UMultiCookerProxy::OnCookProcBegin(FProcWorkerThread* ProcWorker)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::OnCookProcBegin"),FColor::Red);
	// FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker %s Begining"),*ProcWorker->GetThreadName());
}

void UMultiCookerProxy::OnCookProcSuccessed(FProcWorkerThread* ProcWorker)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::OnCookProcSuccessed"),FColor::Red);
	FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker Proc %s Successed!"),*ProcWorker->GetThreadName());
	FString CookerProcName = ProcWorker->GetThreadName();
	UpdateSingleCookerStatus(ProcWorker,true, FAssetsCollection{});
	UpdateMultiCookerStatus();
}

void UMultiCookerProxy::OnCookProcFailed(FProcWorkerThread* ProcWorker)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::OnCookProcFailed"),FColor::Red);
	FScopeLock Lock(&SynchronizationObject);
	FString CookerProcName = ProcWorker->GetThreadName();
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker Proc %s Failure!"),*CookerProcName);
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
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::CreateProcMissionThread"),FColor::Red);
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
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::CreateSingleCookWroker"),FColor::Red);
	FString CurrentConfig;
	UFlibPatchParserHelper::TSerializeStructAsJsonString(SingleCookerSettings,CurrentConfig);
	FString SaveConfigTo = UFlibMultiCookerHelper::GetCookerProcConfigPath(SingleCookerSettings.MissionName,SingleCookerSettings.MissionID);
	
	FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	FString ProfilingCmd = GetSettingObject()->bProfilingPerSingleCooker ? UFlibMultiCookerHelper::GetProfilingCmd() : TEXT("");
	
	FString MissionCommand = FString::Printf(
		TEXT("\"%s\" -run=HotSingleCooker -config=\"%s\" -DDCNOSAVEBOOT -NoAssetRegistryCache -stdout -CrashForUAT -unattended -NoLogTimes -UTF8Output -norenderthread %s %s"),
		*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,
		*ProfilingCmd,
		*GetSettingObject()->GetCombinedAdditionalCommandletArgs()
		);
	
	UE_LOG(LogHotPatcher,Display,TEXT("HotPatcher Cook Mission: %s %s"),*UFlibHotPatcherEditorHelper::GetUECmdBinary(),*MissionCommand);
	auto CookerProcThread = CreateProcMissionThread(UFlibHotPatcherEditorHelper::GetUECmdBinary(),MissionCommand,SingleCookerSettings.MissionName);
	CookerConfigMap.Add(SingleCookerSettings.MissionName,SingleCookerSettings);
	CookerProcessMap.Add(SingleCookerSettings.MissionName,CookerProcThread);
	return CookerProcThread;
}
