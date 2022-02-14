#include "Cooker/MultiCooker/MultiCookerProxy.h"

#include "FlibHotPatcherCoreHelper.h"
#include "Cooker/MultiCooker/FlibMultiCookerHelper.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "Cooker/MultiCooker/MultiCookScheduler.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "ShaderPatch/FlibShaderCodeLibraryHelper.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"

#include "INetworkFileSystemModule.h"
#include "ShaderCompiler.h"
#include "Async/Async.h"


void UMultiCookerProxy::Init(FPatcherEntitySettingBase* InSetting)
{
	Super::Init(InSetting);
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::Init",FColor::Red);
	if(GetSettingObject()->bImportProjectSettings)
	{
		UFlibHotPatcherCoreHelper::ImportProjectSettingsToSettingBase(GetSettingObject());
	}

#if WITH_PACKAGE_CONTEXT
	PlatformSavePackageContexts = UFlibHotPatcherCoreHelper::CreatePlatformsPackageContexts(GetSettingObject()->CookTargetPlatforms,GetSettingObject()->IoStoreSettings.bIoStore);
#endif
	
}

void UMultiCookerProxy::Shutdown()
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::Shutdown",FColor::Red);

	ShutdownShaderCollection();

	SerializeConfig();
	SerializeMultiCookerAssets();
	SerializeCookVersion();

	{
		// serialize cook failed and package track assets
		SCOPED_NAMED_EVENT_TEXT("Serialize Failed & Additional Assets",FColor::Red);
		FPackagePathSet CookFailedSet;
		FPackagePathSet PackageTrackerAdditionalSet;
		
		for(auto PlatformAsset:CookerFailedCollectionMap)
		{
			for(auto& Asset:PlatformAsset.Value.Assets)
			{
				CookFailedSet.PackagePaths.Add(Asset.PackagePath);
			}
		}
		FString CookFailedJsonContent;
		if(!!CookFailedSet.PackagePaths.Num() && THotPatcherTemplateHelper::TSerializeStructAsJsonString(CookFailedSet,CookFailedJsonContent))
		{
			FString SaveTo = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),FString::Printf(TEXT("%s_CookFailedAssets.json"),FApp::GetProjectName()));
			FFileHelper::SaveStringToFile(CookFailedJsonContent,*SaveTo);
		}
		
		for(auto Asset:GetPackageTrackerAssetsByCookers())
		{
			PackageTrackerAdditionalSet.PackagePaths.Add(Asset.PackagePath);
		}
		FString PackageTrackerAdditionalJsonContent;
		if(!!PackageTrackerAdditionalSet.PackagePaths.Num() && THotPatcherTemplateHelper::TSerializeStructAsJsonString(PackageTrackerAdditionalSet,PackageTrackerAdditionalJsonContent))
		{
			FString SaveTo = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),FString::Printf(TEXT("%s_PackageTrackerAssets.json"),FApp::GetProjectName()));
			FFileHelper::SaveStringToFile(PackageTrackerAdditionalJsonContent,*SaveTo);
		}
	}
	Super::Shutdown();
}

bool UMultiCookerProxy::IsRunning() const
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::IsRunning",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::Cancel",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::HasError",FColor::Red);
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
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 25
			RecompileData.SerializedShaderResources,
#endif
			RecompileData.MeshMaterialMaps, 
			RecompileData.ModifiedFiles);
	}
}

void UMultiCookerProxy::OnCookMissionsFinished(bool bSuccessed)
{
	FScopeLock Lock(&SynchronizationObject);
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::OnCookMissionsFinished",FColor::Red);
	if(bSuccessed && GetSettingObject()->ShaderOptions.bMergeShaderLibrary)
	{
		MergeShader();
	}
	
	CookFailedAndAdditionalAssets();
	
	OnMultiCookerFinished.Broadcast(this);
	bMissionFinished = true;
	PostMission();
}

bool UMultiCookerProxy::MergeShader()
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::MergeShader",FColor::Red);
	FString BaseDir = UFlibMultiCookerHelper::GetMultiCookerBaseDir();
	TArray<FString> PlatformNames;
	TArray<FShaderCodeFormatMap> ShaderCodeFormatMaps;
	for(const auto& Platform:GetSettingObject()->CookTargetPlatforms)
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		ITargetPlatform* TargetPlatform = UFlibHotPatcherCoreHelper::GetPlatformByName(PlatformName);
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
				UFlibHotPatcherCoreHelper::DeleteDirectory(Path);
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
			FString CurrentCookerDir = FPaths::Combine(CurrentCookerMetadataDir,PlatformName,TEXT("Metadata/ShaderLibrarySource"));
			
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
					FString ChildShaderInfoFile = FPaths::Combine(CurrentCookerMetadataDir,PlatformName,TEXT(""),ShaderInfoFileName);
					FString CopyToDefaultShaderInfoFile =  UFlibShaderPatchHelper::GetShaderAssetInfoFilename(ShaderIntermediateLocation,FApp::GetProjectName(),ShaderFormat);
					
					FString ChildShaderStableInfoFile = FPaths::Combine(CurrentCookerMetadataDir,PlatformName,TEXT("Metadata/PipelineCaches"),ShaderStableInfoFileName);
					
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

void UMultiCookerProxy::CookFailedAndAdditionalAssets()
{
	FScopeLock Lock(&SynchronizationObject);
	RecookerProxy = NewObject<USingleCookerProxy>();
	RecookerProxy->AddToRoot();
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::CookFailedAndAdditionalAssets",FColor::Red);
	
	FSingleCookerSettings EmptySetting;
	EmptySetting.MissionID = 9999;
	EmptySetting.ShaderLibName = TEXT("AdditionalPackages");
	EmptySetting.MissionName = FString::Printf(TEXT("%s_Cooker_%s"),FApp::GetProjectName(),*EmptySetting.ShaderLibName);
	EmptySetting.CookTargetPlatforms = GetSettingObject()->CookTargetPlatforms;
	EmptySetting.bPackageTracker = false;
	EmptySetting.ShaderOptions.bSharedShaderLibrary = false;
	EmptySetting.ShaderOptions.bNativeShader = false;
	EmptySetting.ShaderOptions.bMergeShaderLibrary = false;
	EmptySetting.IoStoreSettings = GetSettingObject()->IoStoreSettings;
	EmptySetting.IoStoreSettings.bStorageBulkDataInfo = false;
	EmptySetting.bSerializeAssetRegistry = false;
	EmptySetting.bPreGeneratePlatformData = false;
	EmptySetting.bConcurrentSave = false;
	EmptySetting.bDisplayConfig = false;
	EmptySetting.StorageCookedDir = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),TEXT("Cooked"));
#if WITH_PACKAGE_CONTEXT
	EmptySetting.bOverrideSavePackageContext = false;
	EmptySetting.PlatformSavePackageContexts = GetPlatformSavePackageContexts();
#endif
	EmptySetting.ShaderOptions.bSharedShaderLibrary = false;

	EmptySetting.CookAssets.Append(GetPackageTrackerAssetsByCookers());
	EmptySetting.CookAssets.Append(GetCookFailedAssetsByCookers());

	if(RecookerProxy)
	{
		RecookerProxy->Init(&EmptySetting);
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
			FAssetsCollection& FailedCollection = CookerFailedCollectionMap.FindOrAdd(PlatformName);
			FailedCollection = *CookerFailedCollection.CookFailedAssets.Find(TargetPlatform);
			
			UE_LOG(LogHotPatcher,Error,TEXT("\nCook Platfotm %s Assets Failed:\n"),*PlatformName);
			for(const auto& Asset:CookerFailedCollection.CookFailedAssets.Find(TargetPlatform)->Assets)
			{
				UE_LOG(LogHotPatcher,Error,TEXT("\t%s\n"),*Asset.PackagePath.ToString());
			}
		}
	}
}

TArray<FAssetDetail> UMultiCookerProxy::GetCookFailedAssetsByCookers()const
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::GetCookFailedAssetsByCookers",FColor::Red);
	TMap<ETargetPlatform,TArray<FAssetDetail>> AllFailedAssetsMap;
	TArray<FAssetDetail> AllFailedAssets;
	for(const auto& CookFailedCollection:CookerFailedCollectionMap)
	{
		AllFailedAssetsMap.FindOrAdd(CookFailedCollection.Value.TargetPlatform).Append(CookFailedCollection.Value.Assets);
		for(const auto& FailedAsset:CookFailedCollection.Value.Assets)
		{
			AllFailedAssets.AddUnique(FailedAsset);
		}
	}
	return AllFailedAssets;
}

TArray<FAssetDetail> UMultiCookerProxy::GetPackageTrackerAssetsByCookers()const
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::GetPackageTrackerAssetsByCookers",FColor::Red);
	TArray<FAssetDetail> AdditionalAssetDetails;
	FPackagePathSet TotalAdditionPackageSet;
	
	for(const auto& Cooker:CookerConfigMap)
	{
		FPackagePathSet AdditionalPackageSet;
		FString CurrentCookerMetadataDir = Cooker.Value.StorageMetadataDir;
		FString CurrentCookerAdditionPackageSetFile = FPaths::Combine(CurrentCookerMetadataDir,TEXT("AdditionalPackageSet.json"));

		if(FPaths::FileExists(CurrentCookerAdditionPackageSetFile))
		{
			FString JsonContent;
			FFileHelper::LoadFileToString(JsonContent,*CurrentCookerAdditionPackageSetFile);
			if(THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,AdditionalPackageSet))
			{
				for(auto LongPackageName:AdditionalPackageSet.PackagePaths)
				{
					TotalAdditionPackageSet.PackagePaths.Add(LongPackageName);
				}
			}
		}
	}
	for(const auto LongPackageName:TotalAdditionPackageSet.PackagePaths)
	{
		FAssetDetail AssetDetail = UFlibAssetManageHelper::GetAssetDetailByPackageName(LongPackageName.ToString());
		if(AssetDetail.IsValid())
		{
			AdditionalAssetDetails.AddUnique(AssetDetail);
		}
		else
		{
			UE_LOG(LogHotPatcher,Log,TEXT("Get %s FAssetDetail Failed!"),*LongPackageName.ToString());
		}
	}
	return AdditionalAssetDetails;
}

void UMultiCookerProxy::WaitMissionFinished()
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::WaitMissionFinished",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::PreMission",FColor::Red);

	// for cook global shader
	if(GetSettingObject()->bCompileGlobalShader)
	{
		CreateShaderCollectionByName(TEXT("Global"),true);
		SCOPED_NAMED_EVENT_TEXT("Compile Global Shader",FColor::Red);
		// CompileGlobalShader(UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(GetSettingObject()->CookTargetPlatforms));
		SaveGlobalShaderMapFiles(UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(GetSettingObject()->CookTargetPlatforms),UFlibMultiCookerHelper::GetMultiCookerBaseDir());
		UE_LOG(LogHotPatcher,Display,TEXT("MultiCookerProxy: Wait Global Shader Compile Complete!"));
		
		
		ShutdownShaderCollection();
	}
}

void UMultiCookerProxy::PostMission()
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::PostMission",FColor::Red);
	
	FString ShaderLibBaseDir = FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),TEXT("Shaders"));
	
	for(const auto& Platform:GetSettingObject()->CookTargetPlatforms)
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		FString PlatformShaderDir = FPaths::Combine(ShaderLibBaseDir,PlatformName);
		FString PlatformDefaultCookedDir = FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked"),PlatformName);
		FString DefaultCookedProjectContentDir = FPaths::Combine(PlatformDefaultCookedDir,FApp::GetProjectName(),TEXT("Content"));
		
		TArray<FName> ShaderFormats;
		ITargetPlatform* PlatformIns = UFlibHotPatcherCoreHelper::GetTargetPlatformByName(PlatformName);
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

void UMultiCookerProxy::CreateShaderCollectionByName(const FString& Name, bool bCleanDir)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::CreateShaderCollectionByName",FColor::Red);
	GlobalShaderCollectionProxy = UFlibMultiCookerHelper::CreateCookShaderCollectionProxyByPlatform(
		Name,
		GetSettingObject()->CookTargetPlatforms,
		GetSettingObject()->ShaderOptions.bSharedShaderLibrary,
		GetSettingObject()->ShaderOptions.bNativeShader,
		true,
		FPaths::Combine(UFlibMultiCookerHelper::GetMultiCookerBaseDir(),TEXT("Shaders")),
		bCleanDir
	);
	if(GlobalShaderCollectionProxy.IsValid())
	{
		GlobalShaderCollectionProxy->Init();
	}
}

void UMultiCookerProxy::ShutdownShaderCollection()
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::ShutdownShaderCollection",FColor::Red);
	// Wait for all shaders to finish compiling
	UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
	UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
	if(GlobalShaderCollectionProxy.IsValid())
	{
		GlobalShaderCollectionProxy->Shutdown();
	}
}

FExportPatchSettings UMultiCookerProxy::MakePatchSettings()
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::MakePatchSettings",FColor::Red);
	FExportPatchSettings CookPatchSettings;
	CookPatchSettings.AssetIncludeFilters = GetSettingObject()->AssetIncludeFilters;
	CookPatchSettings.AssetIgnoreFilters = GetSettingObject()->AssetIgnoreFilters;
	CookPatchSettings.IncludeSpecifyAssets = GetSettingObject()->IncludeSpecifyAssets;
	CookPatchSettings.ForceSkipClasses = GetSettingObject()->GetForceSkipClasses();
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
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::UpdateMultiCookerStatus",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::UpdateSingleCookerStatus",FColor::Red);
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker %s Mission %s!"),*ProcWorker->GetThreadName(), bSuccessed ? TEXT("Finished") : TEXT("Failed"));
	if(!bSuccessed)
	{
		CookerFailedCollectionMap.FindOrAdd(ProcWorker->GetThreadName()) = FailedCollection;
	}
}


TArray<FSingleCookerSettings> UMultiCookerProxy::MakeSingleCookerSettings(const TArray<FAssetDetail>& AllDetails)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::MakeSingleCookerSettings",FColor::Red);
	UMultiCookScheduler* DefaultScheduler = Cast<UMultiCookScheduler>(GetSettingObject()->Scheduler->GetDefaultObject());
	return DefaultScheduler->MultiCookScheduler(*GetSettingObject(),AllDetails);
}

bool UMultiCookerProxy::DoExport()
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::DoExport",FColor::Red);
	CookerProcessMap.Empty();
	CookerConfigMap.Empty();
	CookerFailedCollectionMap.Empty();
	ScanedCaches.Empty();
	
	FString TempDir = UFlibMultiCookerHelper::GetMultiCookerBaseDir();
	UFlibHotPatcherCoreHelper::DeleteDirectory(TempDir);
	
	PreMission();
	
	// for project shader
	CreateShaderCollectionByName(FApp::GetProjectName(), false);

	CalcCookAssets();
	
	if(!GetSettingObject()->bSkipCook)
	{
		OnMultiCookerBegining.Broadcast(this);
		SCOPED_NAMED_EVENT_TEXT("MakeSingleCookerSettings and Execute",FColor::Red);
		TArray<FSingleCookerSettings> SingleCookerSettings = MakeSingleCookerSettings(GetMultiCookerAssets().Assets);

		for(const auto& CookerSetting:SingleCookerSettings)
		{
			TSharedPtr<FThreadWorker> ProcWorker = CreateSingleCookWorker(CookerSetting);
			ProcWorker->Execute();
		}
	}

	return !HasError();
}

void UMultiCookerProxy::OnOutputMsg(FProcWorkerThread* Worker,const FString& InMsg)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::OnOutputMsg",FColor::Red);
	// FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("%s: %s"),*Worker->GetThreadName(), *InMsg);
}

void UMultiCookerProxy::OnCookProcBegin(FProcWorkerThread* ProcWorker)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::OnCookProcBegin",FColor::Red);
	// FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker %s Begining"),*ProcWorker->GetThreadName());
}

void UMultiCookerProxy::OnCookProcSuccessed(FProcWorkerThread* ProcWorker)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::OnCookProcSuccessed",FColor::Red);
	FScopeLock Lock(&SynchronizationObject);
	UE_LOG(LogHotPatcher,Display,TEXT("Single Cooker Proc %s Successed!"),*ProcWorker->GetThreadName());
	FString CookerProcName = ProcWorker->GetThreadName();
	UpdateSingleCookerStatus(ProcWorker,true, FAssetsCollection{});
	UpdateMultiCookerStatus();
}

void UMultiCookerProxy::OnCookProcFailed(FProcWorkerThread* ProcWorker)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::OnCookProcFailed",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::CreateProcMissionThread",FColor::Red);
	TSharedPtr<FProcWorkerThread> ProcWorkingThread;
	ProcWorkingThread = MakeShareable(new FProcWorkerThread(*MissionName, Bin, Command));
	ProcWorkingThread->ProcOutputMsgDelegate.BindUObject(this,&UMultiCookerProxy::OnOutputMsg);
	ProcWorkingThread->ProcBeginDelegate.AddUObject(this,&UMultiCookerProxy::OnCookProcBegin);
	ProcWorkingThread->ProcSuccessedDelegate.AddUObject(this,&UMultiCookerProxy::OnCookProcSuccessed);
	ProcWorkingThread->ProcFaildDelegate.AddUObject(this,&UMultiCookerProxy::OnCookProcFailed);
	return ProcWorkingThread;
}

TSharedPtr<FProcWorkerThread> UMultiCookerProxy::CreateSingleCookWorker(const FSingleCookerSettings& SingleCookerSettings)
{
	SCOPED_NAMED_EVENT_TEXT("UMultiCookerProxy::CreateSingleCookWroker",FColor::Red);
	FString CurrentConfig;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(SingleCookerSettings,CurrentConfig);
	FString SaveConfigTo = UFlibMultiCookerHelper::GetCookerProcConfigPath(SingleCookerSettings.MissionName,SingleCookerSettings.MissionID);
	
	FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	FString ProfilingCmd = GetSettingObject()->bProfilingPerSingleCooker ? UFlibMultiCookerHelper::GetProfilingCmd() : TEXT("");
	
	FString ShaderPerformanceCmd;
	if(GetSettingObject()->bLocalHostMode)
	{
		const int32 NumVirtualCores = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
		int32 SingleCookerShaderWorker = (int32)((NumVirtualCores - GetSettingObject()->ProcessNumber) / 7);
		SingleCookerShaderWorker = FMath::Max(SingleCookerShaderWorker,3);
		ShaderPerformanceCmd = FString::Printf(TEXT("-MaxShaderWorker=%d"),SingleCookerShaderWorker);
	}
	if(GetSettingObject()->bRealTimeSCWPriority)
	{
		ShaderPerformanceCmd.Append(TEXT(" -rtshaderworker"));
	}
	FString TraceFileCmd;
	if(GetSettingObject()->bUseTraceFile)
	{
		FString TraceFileTo = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),FString::Printf(TEXT("\"%s.utrace\""),*SingleCookerSettings.MissionName));
		TraceFileCmd = FString::Printf(TEXT("-tracefile=\"%s\""),*TraceFileTo);
	}
	
	FString MissionCommand = FString::Printf(
		TEXT("\"%s\" -run=HotSingleCooker -config=\"%s\" -DDCNOSAVEBOOT -NoAssetRegistryCache -stdout -CrashForUAT -unattended -NoLogTimes -UTF8Output -norenderthread -NoCompileGlobalShader %s %s %s %s"),
		*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,
		*ProfilingCmd,
		*ShaderPerformanceCmd,
		*TraceFileCmd,
		*GetSettingObject()->GetCombinedAdditionalCommandletArgs()
		);
	
	UE_LOG(LogHotPatcher,Display,TEXT("HotPatcher Cook Mission: %s %s"),*UFlibHotPatcherCoreHelper::GetUECmdBinary(),*MissionCommand);
	auto CookerProcThread = CreateProcMissionThread(UFlibHotPatcherCoreHelper::GetUECmdBinary(),MissionCommand,SingleCookerSettings.MissionName);
	CookerConfigMap.Add(SingleCookerSettings.MissionName,SingleCookerSettings);
	CookerProcessMap.Add(SingleCookerSettings.MissionName,CookerProcThread);
	return CookerProcThread;
}

void UMultiCookerProxy::SerializeConfig()
{	
	if(GetSettingObject()->IsSaveConfig())
	{
		FString SaveConfigDir = UFlibPatchParserHelper::ReplaceMark(GetSettingObject()->SavePath.Path);
		SCOPED_NAMED_EVENT_TEXT("Save Multi Cooker Config",FColor::Red);
		FString CurrentConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GetSettingObject(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(SaveConfigDir,FString::Printf(TEXT("%s_MultiCookerConfig.json"),FApp::GetProjectName()));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
	}
}

void UMultiCookerProxy::SerializeMultiCookerAssets()
{
	SCOPED_NAMED_EVENT_TEXT("Serialize MultiCookerAssets to json",FColor::Red);
	FString SaveConfigDir = UFlibPatchParserHelper::ReplaceMark(GetSettingObject()->SavePath.Path);
	FString SaveNeedCookAssets;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(GetMultiCookerAssets(),SaveNeedCookAssets);
	FString SaveMultiCookerAssetsTo= FPaths::ConvertRelativePathToFull(SaveConfigDir,FString::Printf(TEXT("%s_MultiCookerAssets.json"),FApp::GetProjectName()));
	FFileHelper::SaveStringToFile(SaveNeedCookAssets,*SaveMultiCookerAssetsTo);
}
void UMultiCookerProxy::SerializeCookVersion()
{
	SCOPED_NAMED_EVENT_TEXT("Serialize MultiCooker CookVersion to json",FColor::Red);
	FString SaveConfigDir = UFlibPatchParserHelper::ReplaceMark(GetSettingObject()->SavePath.Path);
	FString SaveMultiCookerVersionContent;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(GetCookerVersion(),SaveMultiCookerVersionContent);
	FString SaveMultiCookerAssetsTo= FPaths::ConvertRelativePathToFull(SaveConfigDir,FString::Printf(TEXT("%s_MultiCookerVersion_%s.json"),FApp::GetProjectName(),*GetSettingObject()->CookVersionID));
	FFileHelper::SaveStringToFile(SaveMultiCookerVersionContent,*SaveMultiCookerAssetsTo);
}

void UMultiCookerProxy::CalcCookAssets()
{
	// calc need cook assets
	{
		FExportPatchSettings CookPatchSettings = MakePatchSettings();
		FChunkInfo CookChunk = UFlibHotPatcherCoreHelper::MakeChunkFromPatchSettings(&CookPatchSettings);

		FHotPatcherVersion BaseVersion;
		if(GetSettingObject()->bIterateCook)
		{
			FString BaseVersionContent;
			FString BaseVersionFile = UFlibPatchParserHelper::ReplaceMarkPath(GetSettingObject()->RecentCookVersion.FilePath);
			if (UFlibAssetManageHelper::LoadFileToString(BaseVersionFile, BaseVersionContent))
			{
				THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(BaseVersionContent, BaseVersion);
			}
		}
		
		FHotPatcherVersion CurrentVersion;
		{
			SCOPED_NAMED_EVENT_TEXT("ExportReleaseVersionInfoByChunk",FColor::Red);
			CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
			   GetSettingObject()->bIterateCook ? GetSettingObject()->CookVersionID : TEXT("MultiCooker"),
			   GetSettingObject()->bIterateCook ? BaseVersion.VersionId : TEXT(""),
			   FDateTime::UtcNow().ToString(),
			   CookChunk,
			   CookPatchSettings.IsIncludeHasRefAssetsOnly(),
			   CookPatchSettings.IsAnalysisFilterDependencies()
		   );
		}

		FPatchVersionDiff VersionDiffInfo;
		const FAssetDependenciesInfo& BaseVersionAssetDependInfo = BaseVersion.AssetInfo;
		const FAssetDependenciesInfo& CurrentVersionAssetDependInfo = CurrentVersion.AssetInfo;
		
		if(CookPatchSettings.IsForceSkipContent())
		{
			TArray<FString> AllSkipContents;
			AllSkipContents.Append(UFlibAssetManageHelper::DirectoryPathsToStrings(CookPatchSettings.GetForceSkipContentRules()));
			AllSkipContents.Append(UFlibAssetManageHelper::SoftObjectPathsToStrings(CookPatchSettings.GetForceSkipAssets()));
			UFlibAssetManageHelper::ExcludeContentForAssetDependenciesDetail(CurrentVersion.AssetInfo,AllSkipContents);
		}

		UFlibPatchParserHelper::DiffVersionAssets(
			CurrentVersionAssetDependInfo,
			BaseVersionAssetDependInfo,
			VersionDiffInfo.AssetDiffInfo.AddAssetDependInfo,
			VersionDiffInfo.AssetDiffInfo.ModifyAssetDependInfo,
			VersionDiffInfo.AssetDiffInfo.DeleteAssetDependInfo
		);

		if(GetSettingObject()->bIterateCook && GetSettingObject()->bRecursiveWidgetTree)
		{
			UFlibHotPatcherCoreHelper::AnalysisWidgetTree(BaseVersion,CurrentVersion,VersionDiffInfo);
		}
		
		GetMultiCookerAssets().Assets.Append(VersionDiffInfo.AssetDiffInfo.AddAssetDependInfo.GetAssetDetails());
		GetMultiCookerAssets().Assets.Append(VersionDiffInfo.AssetDiffInfo.ModifyAssetDependInfo.GetAssetDetails());

		if(GetSettingObject()->bIterateCook)
		{
			CookVersion = UFlibHotPatcherCoreHelper::MakeNewReleaseByDiff(BaseVersion,VersionDiffInfo,NULL);
		}
		else
		{
			CookVersion = CurrentVersion;
		}
	}
}