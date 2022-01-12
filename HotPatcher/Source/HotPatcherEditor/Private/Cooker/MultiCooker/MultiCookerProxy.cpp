#include "Cooker/MultiCooker/MultiCookerProxy.h"

#include "FlibHotPatcherEditorHelper.h"
#include "FlibMultiCookerHelper.h"
#include "HotPatcherEditor.h"
#include "INetworkFileSystemModule.h"
#include "ShaderCompiler.h"
#include "Cooker/MultiCooker/MultiCookScheduler.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "Async/Async.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"

void UMultiCookerProxy::Init(FPatcherEntitySettingBase* InSetting)
{
	Super::Init(InSetting);
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::Init"),FColor::Red);
	if(GetSettingObject()->bImportProjectSettings)
	{
		UFlibHotPatcherEditorHelper::ImportProjectSettingsToSettingBase(GetSettingObject());
	}
}

void UMultiCookerProxy::Shutdown()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::Shutdown"),FColor::Red);

	ShutdownShaderCollection();
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

void SaveGlobalShaderMapFiles(const TArrayView<const ITargetPlatform* const>& Platforms,const FString& BaseOutputDir)
{
	// we don't support this behavior
	for (int32 Index = 0; Index < Platforms.Num(); Index++)
	{
		// make sure global shaders are up to date!
		TArray<FString> Files;
		FShaderRecompileData RecompileData;
		RecompileData.PlatformName = Platforms[Index]->PlatformName();
		// Compile for all platforms
		RecompileData.ShaderPlatform = -1;
		RecompileData.ModifiedFiles = &Files;
		RecompileData.MeshMaterialMaps = NULL;

		check( IsInGameThread() );

		FString OutputDir  = FPaths::Combine(BaseOutputDir,TEXT("Shaders"),Platforms[Index]->PlatformName());
		RecompileShadersForRemote
			(RecompileData.PlatformName, 
			RecompileData.ShaderPlatform == -1 ? SP_NumPlatforms : (EShaderPlatform)RecompileData.ShaderPlatform, //-V547
			OutputDir, 
			RecompileData.MaterialsToLoad,
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
			RecompileData.ShadersToRecompile,
#endif
			RecompileData.MeshMaterialMaps, 
			RecompileData.ModifiedFiles);
	}
}

// void UMultiCookerProxy::CompileGlobalShader(const TArray<ITargetPlatform*> Platforms)
// {
// 	// don't resave the global shader map files in dlc
// 	if(GetSettingObject()->ShaderOptions.bSharedShaderLibrary)
// 	{
// 		// FShaderCodeLibrary::OpenLibrary(TEXT("Global"), TEXT(""));
// 		SaveGlobalShaderMapFiles(Platforms,UFlibMultiCookerHelper::GetMultiCookerBaseDir());
// 		
// 		// for(const auto& Platform:Platforms)
// 		// {
// 		// 	FString SavePath = FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),TEXT("Shaders"),Platform->PlatformName());
// 		// 	TArray<FName> OutFormats;
// 		// 	Platform->GetAllTargetedShaderFormats(OutFormats);
// 		// 	UFlibShaderCodeLibraryHelper::SaveShaderLibrary(Platform,NULL, TEXT("Global"),SavePath,true);
// 		// }
// 		// FShaderCodeLibrary::CloseLibrary(TEXT("Global"));
// 	}
// }

void UMultiCookerProxy::OnCookMissionsFinished(bool bSuccessed)
{
	FScopeLock Lock(&SynchronizationObject);
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
	bMissionFinished = true;
	PostMission();
}

bool UMultiCookerProxy::MergeShader()
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::MergeShader"),FColor::Red);
	FString BaseDir = UFlibMultiCookerHelper::GetMultiCookerBaseDir();
	TArray<FString> PlatformNames;
	TArray<FShaderCodeFormatMap> ShaderCodeFormatMaps;
	for(const auto& Platform:GetSettingObject()->CookTargetPlatforms)
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		ITargetPlatform* TargetPlatform = UFlibHotPatcherEditorHelper::GetPlatformByName(PlatformName);
		PlatformNames.AddUnique(PlatformName);
		FShaderCodeFormatMap ShaderCodeFormatMap;
		ShaderCodeFormatMap.Platform = TargetPlatform;
		ShaderCodeFormatMap.bIsNative = true;
		ShaderCodeFormatMap.SaveBaseDir = FPaths::Combine(BaseDir,TEXT("Shaders"),PlatformName);

		TArray<FName> ShaderFormats;
		TargetPlatform->GetAllTargetedShaderFormats(ShaderFormats);

		// clear old shader dir
		{
			for(const auto& ShaderFormat:ShaderFormats)
			{
				FString Path = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Shaders"),ShaderFormat.ToString()));
				if(FPaths::DirectoryExists(Path))
				{
					UE_LOG(LogHotPatcher,Display,TEXT("delete shader dir %s"),*Path);
					IFileManager::Get().DeleteDirectory(*Path,true);
				}
			}
		}
		
		// auto SerializeDefaultShaderLib = [&]()
		// {
		// 	for(auto& ShaderFormat:ShaderFormats)
		// 	{
		// 		FString ShaderIntermediateLocation = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Shaders") / ShaderFormat.ToString());
		// 		FString DefaultShaderArchiveLocation = UFlibShaderPatchHelper::GetCodeArchiveFilename(ShaderIntermediateLocation,FApp::GetProjectName(),ShaderFormat);
		// 		FString DefaultShaderInfoLocation = UFlibShaderPatchHelper::GetShaderAssetInfoFilename(ShaderIntermediateLocation,FApp::GetProjectName(),ShaderFormat);
		// 		const FString RootMetaDataPath = ShaderIntermediateLocation / TEXT("Metadata") / TEXT("PipelineCaches");
		// 		UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,TArray<FName>{ShaderFormat},ShaderIntermediateLocation,RootMetaDataPath,true);
		// 	}
		// };
		// load default shader Saved\Shaders\PCD3D_SM5\ShaderArchive-Blank426-PCD3D_SM5.ushaderbytecode
		// SerializeDefaultShaderLib();
		
		for(const auto& Cooker:CookerConfigMap)
		{
			FString CurrentCookerMetadataDir = Cooker.Value.StorageMetadataDir;
			FString CurrentCookerDir = FPaths::Combine(CurrentCookerMetadataDir,Cooker.Key,PlatformName,TEXT("Metadata/ShaderLibrarySource"));
			
			TArray<FString> PlatformSCLCSVPaths;
			for(auto& ShaderFormat:ShaderFormats)
			{
				FString ShaderIntermediateLocation = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Shaders") / ShaderFormat.ToString());
				FString DefaultShaderArchiveLocation = UFlibShaderPatchHelper::GetCodeArchiveFilename(ShaderIntermediateLocation,FApp::GetProjectName(),ShaderFormat);
				FString DefaultShaderInfoLocation = UFlibShaderPatchHelper::GetShaderAssetInfoFilename(ShaderIntermediateLocation,FApp::GetProjectName(),ShaderFormat);
				FString DefaultShaderStableInfoFile =  UFlibShaderPatchHelper::GetStableInfoArchiveFilename(FPaths::Combine(ShaderIntermediateLocation,TEXT("Metadata/PipelineCaches")),FApp::GetProjectName(),ShaderFormat);
				// const FString RootMetaDataPath = ShaderIntermediateLocation / TEXT("Metadata") / TEXT("PipelineCaches");
				// UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,TArray<FName>{ShaderFormat},ShaderIntermediateLocation,RootMetaDataPath,true);
				
				TArray<FString > FoundShaderFiles = UFlibShaderCodeLibraryHelper::FindCookedShaderLibByShaderFrmat(ShaderFormat.ToString(),CurrentCookerDir);
				for(const auto& ShaderArchiveFileName:FoundShaderFiles)
				{
					FString ShaderInfoFileName = UFlibShaderPatchHelper::GetShaderInfoFileNameByShaderArchiveFileName(ShaderArchiveFileName);
					FString ShaderStableInfoFileName = UFlibShaderPatchHelper::GetShaderStableInfoFileNameByShaderArchiveFileName(ShaderArchiveFileName);
					
					FString CopyShaderInfoFileTo = UFlibShaderPatchHelper::GetShaderAssetInfoFilename(ShaderCodeFormatMap.SaveBaseDir,FApp::GetProjectName(),ShaderFormat);
					FString ChildShaderInfoFile = FPaths::Combine(CurrentCookerMetadataDir,Cooker.Key,PlatformName,TEXT(""),ShaderInfoFileName);
					FString CopyToDefaultShaderInfoFile =  UFlibShaderPatchHelper::GetShaderAssetInfoFilename(ShaderIntermediateLocation,FApp::GetProjectName(),ShaderFormat);
					
					FString ChildShaderStableInfoFile = FPaths::Combine(CurrentCookerMetadataDir,Cooker.Key,PlatformName,TEXT("Metadata/PipelineCaches"),ShaderStableInfoFileName);
					
					FString CopyShaderArchiveTo = UFlibShaderPatchHelper::GetCodeArchiveFilename(ShaderCodeFormatMap.SaveBaseDir,FApp::GetProjectName(),ShaderFormat);
					FString ChildShaderArchive = FPaths::Combine(CurrentCookerDir,ShaderArchiveFileName);

					FString CopyShaderSclCsvTo = UFlibShaderPatchHelper::GetStableInfoArchiveFilename(FPaths::Combine(ShaderCodeFormatMap.SaveBaseDir,TEXT("Metadata/PipelineCaches")),FApp::GetProjectName(),ShaderFormat);
					IFileManager::Get().Copy(*CopyShaderArchiveTo,*ChildShaderArchive);
					IFileManager::Get().Copy(*CopyShaderInfoFileTo,*ChildShaderInfoFile);
					IFileManager::Get().Copy(*CopyShaderSclCsvTo,*ChildShaderStableInfoFile);
					// copy to Saved/Shaders
					IFileManager::Get().Copy(*DefaultShaderArchiveLocation,*ChildShaderArchive);
					// IFileManager::Get().Copy(*DefaultShaderInfoLocation,*ChildShaderInfoFile);
					// IFileManager::Get().Copy(*CopyToDefaultShaderInfoFile,*ChildShaderInfoFile);
					// IFileManager::Get().Copy(*DefaultShaderStableInfoFile,*ChildShaderStableInfoFile);
				}
			}
			FString ShaderCodeDir = ShaderCodeFormatMap.SaveBaseDir; //FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),Cooker.Value.MissionName,PlatformName);
			const FString RootMetaDataPath = ShaderCodeDir / TEXT("Metadata") / TEXT("PipelineCaches");

			UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,ShaderFormats,ShaderCodeDir,RootMetaDataPath,true);
			// Save to default shader Saved\Shaders\PCD3D_SM5\ShaderArchive-Blank426-PCD3D_SM5.ushaderbytecode
			
		}
		FString ShaderDir = FPaths::Combine(BaseDir,UFlibMultiCookerHelper::GetMultiCookerBaseDir(),TEXT("Shader"),PlatformName);
		const FString RootMetaDataPath = ShaderDir / TEXT("Metadata") / TEXT("PipelineCaches");;
		UFlibShaderCodeLibraryHelper::SaveShaderLibrary(TargetPlatform,ShaderFormats,ShaderDir,RootMetaDataPath,true);
		// SerializeDefaultShaderLib();
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
	SingleCookerSetting.ShaderOptions.bSharedShaderLibrary = false;
	TArray<FAssetDetail>& AllFailedAssets = SingleCookerSetting.CookAssets;
	for(const auto& CookFailedCollection:CookerFailedCollectionMap)
	{
		AllFailedAssetsMap.FindOrAdd(CookFailedCollection.Value.TargetPlatform).Append(CookFailedCollection.Value.Assets);
		for(const auto& FailedAsset:CookFailedCollection.Value.Assets)
		{
			AllFailedAssets.AddUnique(FailedAsset);
		}
	}
	
	AllFailedAssetsMap.GetKeys(SingleCookerSetting.CookTargetPlatforms);

	if(RecookerProxy)
	{
		RecookerProxy->Init(&SingleCookerSetting);
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
			FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(TargetPlatform);
			CookerFailedCollectionMap.FindOrAdd(PlatformName,*CookerFailedCollection.CookFailedAssets.Find(TargetPlatform));
			UE_LOG(LogHotPatcher,Error,TEXT("\nCook Platfotm %s Assets Failed:\n"),*PlatformName);
			for(const auto& Asset:CookerFailedCollection.CookFailedAssets.Find(TargetPlatform)->Assets)
			{
				UE_LOG(LogHotPatcher,Error,TEXT("\t%s\n"),*Asset.PackagePath.ToString());
			}
		}
	}
}

void UMultiCookerProxy::WaitMissionFinished()
{
	TSharedPtr<FThreadWorker> WaitThreadWorker = MakeShareable(new FThreadWorker(TEXT("SingleCooker_WaitCookComplete"),[this]()
		{
			while(IsRunning())
			{
				FPlatformProcess::Sleep(1.f);
			}
		}));
	WaitThreadWorker->Execute();
	WaitThreadWorker->Join();
	OnCookMissionsFinished(!HasError());
}

void UMultiCookerProxy::PreMission()
{
	
}

void UMultiCookerProxy::PostMission()
{
	FString ShaderLibBaseDir = FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),TEXT("Shaders"));
	
	for(const auto& Platform:GetSettingObject()->CookTargetPlatforms)
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		FString PlatformShaderDir = FPaths::Combine(ShaderLibBaseDir,PlatformName);
		FString PlatformDefaultCookedDir = FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked"),PlatformName);
		FString DefaultCookedProjectContentDir = FPaths::Combine(PlatformDefaultCookedDir,FApp::GetProjectName(),TEXT("Content"));
		
		TArray<FName> ShaderFormats;
		ITargetPlatform* PlatformIns = UFlibHotPatcherEditorHelper::GetTargetPlatformByName(PlatformName);
		if(PlatformIns)
		{
			FString CookedMetadataDir = FPaths::Combine(PlatformDefaultCookedDir,FApp::GetProjectName(),TEXT("Metadata/ShaderLibrarySource"));
			FPaths::NormalizeFilename(CookedMetadataDir);
			PlatformIns->GetAllTargetedShaderFormats(ShaderFormats);
			for(const auto& ShaderFormat:ShaderFormats)
			{
				// copy GlobalShaderCache-%s.bin to Saved/Cooked/WindowsNoEditor/PROJECT_NAME/
				FString GlobalShaderCache = FPaths::Combine(PlatformShaderDir,FString::Printf(TEXT("Engine/GlobalShaderCache-%s.bin"),*ShaderFormat.ToString()));
				FString DefaultGlobalShaderCache = FPaths::Combine(PlatformDefaultCookedDir,FString::Printf(TEXT("Engine/GlobalShaderCache-%s.bin"),*ShaderFormat.ToString()));
				IFileManager::Get().Copy(*DefaultGlobalShaderCache,*GlobalShaderCache,true,true);

				// copy ShaderArchive-Global-PCD3D_SM5.ushaderbytecode to Saved/Cooked/WindowsNoEditor/PROJECT_NAME/Content
				FString GlobalShaderLib = UFlibShaderPatchHelper::GetCodeArchiveFilename(PlatformShaderDir,TEXT("Global"),ShaderFormat);
				FString DefaultGlobalShaderLib = UFlibShaderPatchHelper::GetCodeArchiveFilename(DefaultCookedProjectContentDir ,TEXT("Global"),ShaderFormat);
				FString MetadateGlobalShaderLib = UFlibShaderPatchHelper::GetCodeArchiveFilename(CookedMetadataDir,TEXT("Global"),ShaderFormat);
				IFileManager::Get().Copy(*DefaultGlobalShaderLib,*GlobalShaderLib,true,true);
				IFileManager::Get().Copy(*MetadateGlobalShaderLib,*GlobalShaderLib,true,true);
				
				// copy ShaderArchive-PROJECT_NAME-PCD3D_SM5.ushaderbytecode to Saved/Cooked/WindowsNoEditor/PROJECT_NAME/Content
				FString ProjectShaderLib = UFlibShaderPatchHelper::GetCodeArchiveFilename(PlatformShaderDir,FApp::GetProjectName(),ShaderFormat);
				FString DefaultShaderLib = UFlibShaderPatchHelper::GetCodeArchiveFilename(DefaultCookedProjectContentDir,FApp::GetProjectName(),ShaderFormat);
				FString MetadateProjectShaderLib = UFlibShaderPatchHelper::GetCodeArchiveFilename(CookedMetadataDir,FApp::GetProjectName(),ShaderFormat);
				IFileManager::Get().Copy(*DefaultShaderLib,*ProjectShaderLib,true,true);
				IFileManager::Get().Copy(*MetadateProjectShaderLib,*ProjectShaderLib,true,true);
			}
			
			FString AssetInfoJsonDir = FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),TEXT("Shaders"),PlatformName);
			TArray<FString > AssetInfoJsonFiles;
			IFileManager::Get().FindFiles(AssetInfoJsonFiles,*AssetInfoJsonDir,TEXT("assetinfo.json"));
			for(const auto& AssetInfoJsonFileName:AssetInfoJsonFiles)
			{
				FString SrcFile = FPaths::Combine(AssetInfoJsonDir,AssetInfoJsonFileName);
				FString CopyTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked"),PlatformName,FApp::GetProjectName(),TEXT("Content"),AssetInfoJsonFileName));
				IFileManager::Get().Copy(*CopyTo,*SrcFile,true,true);
			}
		}
	}
}

void UMultiCookerProxy::CreateShaderCollectionByName(const FString& Name)
{
	GlobalShaderCollectionProxy = UFlibMultiCookerHelper::CreateCookShaderCollectionProxyByPlatform(
	Name,
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
}

void UMultiCookerProxy::ShutdownShaderCollection()
{
	if(GlobalShaderCollectionProxy.IsValid())
	{
		GlobalShaderCollectionProxy->Shutdown();
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
	CookPatchSettings.ForceSkipAssets = GetSettingObject()->ForceSkipAssets;
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
		if(!IsRunningCommandlet())
		{
			AsyncTask(ENamedThreads::GameThread,[this]()
			{
				OnCookMissionsFinished(!HasError());
			});
		}
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

	PreMission();
	// for cook global shader
	if(GetSettingObject()->bCompileGlobalShader)
	{
		CreateShaderCollectionByName(TEXT("Global"));
		SCOPED_NAMED_EVENT_TCHAR(TEXT("UMultiCookerProxy::DoExport Compile Global Shader"),FColor::Red);
		// CompileGlobalShader(UFlibHotPatcherEditorHelper::GetTargetPlatformsByNames(GetSettingObject()->CookTargetPlatforms));
		SaveGlobalShaderMapFiles(UFlibHotPatcherEditorHelper::GetTargetPlatformsByNames(GetSettingObject()->CookTargetPlatforms),UFlibMultiCookerHelper::GetMultiCookerBaseDir());
		UE_LOG(LogHotPatcher,Display,TEXT("MultiCookerProxy: Wait Global Shader Compile complate!"));
		
		// Wait for all shaders to finish compiling
		UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplate();
		ShutdownShaderCollection();
	}

	// for project shader
	CreateShaderCollectionByName(FApp::GetProjectName());
	
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
		   CookPatchSettings.IsIncludeHasRefAssetsOnly(),
		   CookPatchSettings.IsAnalysisFilterDependencies()
	   );
	}
	
	if(CookPatchSettings.IsForceSkipContent())
	{
		TArray<FString> AllSkipContents;
		AllSkipContents.Append(UFlibAssetManageHelper::DirectoryPathsToStrings(CookPatchSettings.GetForceSkipContentRules()));
		AllSkipContents.Append(UFlibAssetManageHelper::SoftObjectPathsToStrings(CookPatchSettings.GetForceSkipAssets()));
		UFlibAssetManageHelper::ExcludeContentForAssetDependenciesDetail(CurrentVersion.AssetInfo,AllSkipContents);
	}
	
	FMultiCookerAssets MultiCookerAssets;
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("GetAssetDetailsByAssetDependenciesInfo and Serialize to json"),FColor::Red);
		MultiCookerAssets.Assets = CurrentVersion.AssetInfo.GetAssetDetails();
		FString SaveNeedCookAssets;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(MultiCookerAssets,SaveNeedCookAssets);
		FString SaveMultiCookerAssetsTo= FPaths::ConvertRelativePathToFull(SaveConfigDir,FString::Printf(TEXT("%s_MultiCookerAssets.json"),FApp::GetProjectName()));
		FFileHelper::SaveStringToFile(SaveNeedCookAssets,*SaveMultiCookerAssetsTo);
	}
	
	if(!GetSettingObject()->bSkipCook)
	{
		OnMultiCookerBegining.Broadcast(this);
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
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GetSettingObject(),CurrentConfig);
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
	if(ProcWorker && CookerConfigMap.Contains(ProcWorker->GetThreadName()))
	{
		FSingleCookerSettings* CookerSetting = CookerConfigMap.Find(ProcWorker->GetThreadName());
		FScopeLock Lock(&SynchronizationObject);
		FString CookerProcName = ProcWorker->GetThreadName();
		UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker Proc %s Failure!"),*CookerProcName);
		FSingleCookerSettings* CookerSettings =  CookerConfigMap.Find(CookerProcName);
		FString CookFailedResultPath = UFlibMultiCookerHelper::GetCookerProcFailedResultPath(CookerSetting->StorageMetadataDir,CookerSettings->MissionName,CookerSettings->MissionID);
		FAssetsCollection FailedMissionCollection;
		if(FPaths::FileExists(CookFailedResultPath))
		{
			FString FailedContent;
			if(FFileHelper::LoadFileToString(FailedContent,*CookFailedResultPath))
			{
				THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(FailedContent,FailedMissionCollection);
			}
		}
		// CookerProcessMap.Remove(CookerProcName);
		UpdateSingleCookerStatus(ProcWorker,false, FailedMissionCollection);
		UpdateMultiCookerStatus();
	}
	
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
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(SingleCookerSettings,CurrentConfig);
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
