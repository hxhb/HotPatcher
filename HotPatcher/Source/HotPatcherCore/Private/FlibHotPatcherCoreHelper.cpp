// Fill out your copyright notice in the Description page of Project Settings.

#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherLog.h"
#include "HotPatcherCore.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "ShaderLibUtils/FlibShaderCodeLibraryHelper.h"
#include "ThreadUtils/FThreadUtils.hpp"

// engine header
#include "Misc/ConfigCacheIni.h"
#include "Editor.h"
#include "Interfaces/ITargetPlatform.h"
#include "HAL/PlatformFilemanager.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "GameDelegates.h"
#include "GameMapsSettings.h"
#include "INetworkFileSystemModule.h"
#include "IPlatformFileSandboxWrapper.h"
#include "PackageHelperFunctions.h"
#include "Async/Async.h"
#include "Engine/AssetManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/RedirectCollector.h"
#include "Misc/SecureHash.h"
#include "Serialization/ArrayWriter.h"
#include "Settings/ProjectPackagingSettings.h"
#include "ShaderCompiler.h"
#include "Async/ParallelFor.h"
#include "CreatePatch/PatcherProxy.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ProfilingDebugging/LoadTimeTracker.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/CoreMisc.h"
#include "DerivedDataCacheInterface.h"


DEFINE_LOG_CATEGORY(LogHotPatcherCoreHelper);

TArray<FString> UFlibHotPatcherCoreHelper::GetAllCookOption()
{
	TArray<FString> result
	{
		"Iterate",
		"UnVersioned",
		"CookAll",
		"Compressed"
	};
	return result;
}

void UFlibHotPatcherCoreHelper::CheckInvalidCookFilesByAssetDependenciesInfo(
	const FString& InProjectAbsDir,
	const FString& OverrideCookedDir,
	const FString& InPlatformName, 
	const FAssetDependenciesInfo& InAssetDependencies, 
	TArray<FAssetDetail>& OutValidAssets, 
	TArray<FAssetDetail>& OutInvalidAssets)
{
	OutValidAssets.Empty();
	OutInvalidAssets.Empty();
	const TArray<FAssetDetail>& AllAssetDetails = InAssetDependencies.GetAssetDetails();

	for (const auto& AssetDetail : AllAssetDetails)
	{
		TArray<FString> CookedAssetPath;
		TArray<FString> CookedAssetRelativePath;
		FString AssetLongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(AssetDetail.PackagePath.ToString());
		
		FAssetData CurrentAssetData;
		UFlibAssetManageHelper::GetSingleAssetsData(AssetDetail.PackagePath.ToString(),CurrentAssetData);
		
		// if(!CurrentAssetData.GetAsset()->IsValidLowLevelFast())
		// {
		// 	UE_LOG(LogHotPatcherCoreHelper,Warning,TEXT("%s is invalid Asset Uobject"),*CurrentAssetData.PackageName.ToString());
		// 	continue;
		// }
		if ((CurrentAssetData.PackageFlags & PKG_EditorOnly)!=0)
		{
			UE_LOG(LogHotPatcherCoreHelper,Warning,TEXT("Miss %s it's EditorOnly Assets!"),*CurrentAssetData.PackageName.ToString());
			continue;
		}
		
		if (UFlibAssetManageHelper::ConvLongPackageNameToCookedPath(
			InProjectAbsDir,
			InPlatformName,
			AssetLongPackageName,
			CookedAssetPath,
			CookedAssetRelativePath,
			OverrideCookedDir
			))
		{
			if (CookedAssetPath.Num() > 0)
			{
				OutValidAssets.Add(AssetDetail);
			}
			else
			{
				OutInvalidAssets.Add(AssetDetail);
			}
		}
	}
}


#include "Kismet/KismetSystemLibrary.h"

FChunkInfo UFlibHotPatcherCoreHelper::MakeChunkFromPatchSettings(const FExportPatchSettings* InPatchSetting)
{
	FChunkInfo Chunk;
	if (!(InPatchSetting && InPatchSetting))
	{
		return Chunk;
	}
	auto GetThis = [&]()->FExportPatchSettings*{return const_cast<FExportPatchSettings*>(InPatchSetting);};
	Chunk.ChunkName = InPatchSetting->VersionId;
	Chunk.bMonolithic = false;
	Chunk.MonolithicPathMode = EMonolithicPathMode::MountPath;
	Chunk.bStorageUnrealPakList = InPatchSetting->GetUnrealPakSettings().bStoragePakList;
	Chunk.bStorageIoStorePakList = InPatchSetting->GetIoStoreSettings().bStoragePakList;
	Chunk.bOutputDebugInfo = Chunk.bStorageIoStorePakList || Chunk.bStorageUnrealPakList;
	Chunk.AssetIncludeFilters = GetThis()->GetAssetIncludeFilters();
	Chunk.AssetIgnoreFilters = GetThis()->GetAssetIgnoreFilters();
	Chunk.bAnalysisFilterDependencies = InPatchSetting->IsAnalysisFilterDependencies();
	Chunk.IncludeSpecifyAssets = GetThis()->GetIncludeSpecifyAssets();
	Chunk.bForceSkipContent = GetThis()->IsForceSkipContent();
	Chunk.ForceSkipClasses = GetThis()->GetAssetScanConfig().ForceSkipClasses;
	Chunk.ForceSkipContentRules = GetThis()->GetForceSkipContentRules();
	Chunk.ForceSkipAssets = GetThis()->GetForceSkipAssets();
	Chunk.AddExternAssetsToPlatform = GetThis()->GetAddExternAssetsToPlatform();
	Chunk.AssetRegistryDependencyTypes = InPatchSetting->GetAssetRegistryDependencyTypes();
	Chunk.InternalFiles.bIncludeAssetRegistry = InPatchSetting->IsIncludeAssetRegistry();
	Chunk.InternalFiles.bIncludeGlobalShaderCache = InPatchSetting->IsIncludeGlobalShaderCache();
	Chunk.InternalFiles.bIncludeShaderBytecode = InPatchSetting->IsIncludeShaderBytecode();
	Chunk.InternalFiles.bIncludeEngineIni = InPatchSetting->IsIncludeEngineIni();
	Chunk.InternalFiles.bIncludePluginIni = InPatchSetting->IsIncludePluginIni();
	Chunk.InternalFiles.bIncludeProjectIni = InPatchSetting->IsIncludeProjectIni();
	Chunk.CookShaderOptions = InPatchSetting->GetCookShaderOptions();
	return Chunk;
}

FChunkInfo UFlibHotPatcherCoreHelper::MakeChunkFromPatchVerison(const FHotPatcherVersion& InPatchVersion)
{
	FChunkInfo Chunk;
	Chunk.ChunkName = InPatchVersion.VersionId;
	Chunk.bMonolithic = false;
	Chunk.bStorageUnrealPakList = false;
	auto ConvPathStrToDirPaths = [](const TArray<FString>& InPathsStr)->TArray<FDirectoryPath>
	{
		TArray<FDirectoryPath> result;
		for (const auto& Dir : InPathsStr)
		{
			FDirectoryPath Path;
			Path.Path = Dir;
			result.Add(Path);
		}
		return result;
	};

	//Chunk.AssetIncludeFilters = ConvPathStrToDirPaths(InPatchVersion.IgnoreFilter);
	// Chunk.AssetIgnoreFilters = ConvPathStrToDirPaths(InPatchVersion.IgnoreFilter);
	Chunk.bAnalysisFilterDependencies = false;
	const TArray<FAssetDetail>& AllVersionAssets = InPatchVersion.AssetInfo.GetAssetDetails();

	for (const auto& Asset : AllVersionAssets)
	{
		FPatcherSpecifyAsset CurrentAsset;
		CurrentAsset.Asset = FSoftObjectPath(Asset.PackagePath.ToString());
		CurrentAsset.bAnalysisAssetDependencies = false;
		Chunk.IncludeSpecifyAssets.AddUnique(CurrentAsset);
	}
	// Chunk.AddExternDirectoryToPak = InPatchSetting->GetAddExternDirectory();
	// for (const auto& File : InPatchVersion.ExternalFiles)
	// {
	// 	Chunk.AddExternFileToPak.AddUnique(File.Value);
	// }

	TArray<ETargetPlatform> VersionPlatforms;

	InPatchVersion.PlatformAssets.GetKeys(VersionPlatforms);

	for(auto Platform:VersionPlatforms)
	{
		Chunk.AddExternAssetsToPlatform.Add(InPatchVersion.PlatformAssets[Platform]);
	}
	
	Chunk.InternalFiles.bIncludeAssetRegistry = false;
	Chunk.InternalFiles.bIncludeGlobalShaderCache = false;
	Chunk.InternalFiles.bIncludeShaderBytecode = false;
	Chunk.InternalFiles.bIncludeEngineIni = false;
	Chunk.InternalFiles.bIncludePluginIni = false;
	Chunk.InternalFiles.bIncludeProjectIni = false;

	return Chunk;
}

#define REMAPPED_PLUGGINS TEXT("RemappedPlugins")


FString ConvertToFullSandboxPath( const FString &FileName, bool bForWrite )
{
	FString Result;
	static FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir());
	static FString ProjectContentAbsir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

	if(FileName.StartsWith(ProjectContentAbsir))
	{
		FString GameFileName = FileName;
		GameFileName.RemoveFromStart(ProjectContentAbsir);
		Result = FPaths::Combine(FApp::GetProjectName(),TEXT("Content"),GameFileName);
		return Result;
	}
	if(FileName.StartsWith(EngineAbsDir))
	{
		FString EngineFileName = FileName;
		EngineFileName.RemoveFromStart(EngineAbsDir);
		Result = FPaths::Combine(TEXT("Engine/Content"),EngineFileName);
		return Result;
	}
	TArray<TSharedRef<IPlugin> > PluginsToRemap = IPluginManager::Get().GetEnabledPlugins();
	// Ideally this would be in the Sandbox File but it can't access the project or plugin
	if (PluginsToRemap.Num() > 0)
	{
		// Handle remapping of plugins
		for (TSharedRef<IPlugin> Plugin : PluginsToRemap)
		{
			FString PluginContentDir;
			if (FPaths::IsRelative(FileName))
				PluginContentDir = Plugin->GetContentDir();
			else
				PluginContentDir = FPaths::ConvertRelativePathToFull(Plugin->GetContentDir());
			// UE_LOG(LogHotPatcherCoreHelper,Log,TEXT("Plugin Content:%s"),*PluginContentDir);
			if (FileName.StartsWith(PluginContentDir))
			{
				FString LoadingFrom;
				FString BasePath;
				switch(Plugin->GetLoadedFrom())
				{
				case EPluginLoadedFrom::Engine:
					{
						BasePath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());//TEXT("Engine/Plugins");
						LoadingFrom = TEXT("Engine");
						break;
					}
				case EPluginLoadedFrom::Project:
					{
						BasePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());//FPaths::Combine(FApp::GetProjectName(),TEXT("Plugins"));
						LoadingFrom = FApp::GetProjectName();
						break;
					}
				}
				FString AssetAbsPath = FPaths::ConvertRelativePathToFull(FileName);
				if(AssetAbsPath.StartsWith(BasePath))
				{
					Result = LoadingFrom / AssetAbsPath.RightChop(BasePath.Len());
				}
			}
		}
	}

	return Result;
}

FString UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(const FString& BaseDir, const FString PacakgeName, const FString& Platform)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibHotPatcherCoreHelper::GetCookAssetsSaveDir",FColor::Red);
	FString CookDir;
	FString Filename;
	// FString PackageFilename;
	FString StandardFilename;
	FName StandardFileFName = NAME_None;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (FPackageName::DoesPackageExist(PacakgeName,NULL, &Filename, false))
	{
		StandardFilename = FPaths::ConvertRelativePathToFull(Filename);
		FString SandboxFilename = ConvertToFullSandboxPath(*StandardFilename, true);
		CookDir = FPaths::Combine(BaseDir,Platform,SandboxFilename);
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	
	return 	CookDir;
}

FString UFlibHotPatcherCoreHelper::GetProjectCookedDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));;
}

#if WITH_PACKAGE_CONTEXT
// engine header
#include "UObject/SavePackage.h"

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
#include "Serialization/BulkDataManifest.h"
#endif

#if ENGINE_MAJOR_VERSION > 4 /*&& ENGINE_MINOR_VERSION > 0 */
#include "ZenStoreWriter.h"
#include "Cooker/PackageWriter/HotPatcherPackageWriter.h"
#endif



FSavePackageContext* UFlibHotPatcherCoreHelper::CreateSaveContext(const ITargetPlatform* TargetPlatform,
	bool bUseZenLoader,
	const FString& OverrideCookedDir
	)
{
	FSavePackageContext* SavePackageContext = NULL;
#if WITH_PACKAGE_CONTEXT
	const FString PlatformString = TargetPlatform->PlatformName();

	// const FString ResolvedRootPath = RootPathSandbox.Replace(TEXT("[Platform]"), *PlatformString);
	const FString ResolvedProjectPath = FPaths::Combine(OverrideCookedDir,FString::Printf(TEXT("%s/%s"),*TargetPlatform->PlatformName(),FApp::GetProjectName()));
	const FString ResolvedMetadataPath = FPaths::Combine(ResolvedProjectPath,TEXT("Mededata"));
	
	FConfigFile PlatformEngineIni;
	FConfigCacheIni::LoadLocalIniFile(PlatformEngineIni, TEXT("Engine"), true, *TargetPlatform->IniPlatformName());
	
	
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
	FPackageStoreBulkDataManifest* BulkDataManifest	= new FPackageStoreBulkDataManifest(ResolvedProjectPath);
	FLooseFileWriter* LooseFileWriter				= bUseZenLoader ? new FLooseFileWriter() : nullptr;
	bool bLegacyBulkDataOffsets = false;
	PlatformEngineIni.GetBool(TEXT("Core.System"), TEXT("LegacyBulkDataOffsets"), bLegacyBulkDataOffsets);
	SavePackageContext	= new FSavePackageContext(LooseFileWriter, BulkDataManifest, bLegacyBulkDataOffsets);
#endif
	
#if ENGINE_MAJOR_VERSION > 4 /*&& ENGINE_MINOR_VERSION > 0*/
	ICookedPackageWriter* PackageWriter = nullptr;
	FString WriterDebugName;
	if (bUseZenLoader)
	{
		PackageWriter = new FZenStoreWriter(ResolvedProjectPath, ResolvedMetadataPath, TargetPlatform);
		WriterDebugName = TEXT("ZenStore");
	}
	else
	{
		// FAsyncIODelete AsyncIODelete{ResolvedProjectPath};
		PackageWriter = new FHotPatcherPackageWriter;// new FLooseCookedPackageWriter(ResolvedProjectPath, ResolvedMetadataPath, TargetPlatform,AsyncIODelete,FPackageNameCache{},IPluginManager::Get().GetEnabledPlugins());
		WriterDebugName = TEXT("DirectoryWriter");
	}
	
	SavePackageContext	= new FSavePackageContext(TargetPlatform, PackageWriter);
#endif
#endif
	return SavePackageContext;
}


TMap<ETargetPlatform, TSharedPtr<FSavePackageContext>> UFlibHotPatcherCoreHelper::CreatePlatformsPackageContexts(
	const TArray<ETargetPlatform>& Platforms,bool bIoStore,const FString& OverrideCookedDir)

{
	SCOPED_NAMED_EVENT_TEXT("CreatePlatformsPackageContexts",FColor::Red);

	TMap<ETargetPlatform, TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
	TArray<FString> PlatformNames;

	for(ETargetPlatform Platform:Platforms)
	{
		PlatformNames.AddUnique(THotPatcherTemplateHelper::GetEnumNameByValue(Platform));
	}
	
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	TArray<ITargetPlatform*> CookPlatforms;
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		if (PlatformNames.Contains(TargetPlatform->PlatformName()))
		{
			CookPlatforms.AddUnique(TargetPlatform);
			const FString PlatformString = TargetPlatform->PlatformName();
			
			ETargetPlatform Platform;
			THotPatcherTemplateHelper::GetEnumValueByName(TargetPlatform->PlatformName(),Platform);
			PlatformSavePackageContexts.Add(Platform,MakeShareable(UFlibHotPatcherCoreHelper::CreateSaveContext(TargetPlatform,bIoStore,OverrideCookedDir)));
		}
	}
	return PlatformSavePackageContexts;
}


bool UFlibHotPatcherCoreHelper::SavePlatformBulkDataManifest(TMap<ETargetPlatform, TSharedPtr<FSavePackageContext>>&PlatformSavePackageContexts,ETargetPlatform Platform)
{
	SCOPED_NAMED_EVENT_TEXT("SavePlatformBulkDataManifest",FColor::Red);
	bool bRet = false;
	if(!PlatformSavePackageContexts.Contains(Platform))
		return bRet;
	TSharedPtr<FSavePackageContext> PackageContext = *PlatformSavePackageContexts.Find(Platform);
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION > 25
	if (PackageContext != nullptr && PackageContext->BulkDataManifest != nullptr)
	{
		PackageContext->BulkDataManifest->Save();
		bRet = true;
	}
#endif
	return bRet;
}
#endif

void UFlibHotPatcherCoreHelper::CookAssets(
	const TArray<FSoftObjectPath>& Assets,
	const TArray<ETargetPlatform>& Platforms,
	FCookActionCallback CookActionCallback,
#if WITH_PACKAGE_CONTEXT
	class TMap<ETargetPlatform, FSavePackageContext*> PlatformSavePackageContext,
#endif
	const FString& InSavePath
)
{
	SCOPED_NAMED_EVENT_TEXT("CookAssets",FColor::Red);
	TArray<FString> StringPlatforms;
#if WITH_PACKAGE_CONTEXT
	TMap<FString,FSavePackageContext*> FinalPlatformSavePackageContext;
#endif
	TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms; 
	for(const auto& Platform:Platforms)
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		StringPlatforms.AddUnique(PlatformName);
#if WITH_PACKAGE_CONTEXT
		FSavePackageContext* CurrentPackageContext = NULL;
		if(PlatformSavePackageContext.Contains(Platform))
		{
			CurrentPackageContext = *PlatformSavePackageContext.Find(Platform);
		}
		 
		FinalPlatformSavePackageContext.Add(PlatformName,CurrentPackageContext);
#endif
		ITargetPlatform* PlatformIns = GetPlatformByName(PlatformName);
		if(PlatformIns)
		{
			CookPlatforms.Add(Platform,PlatformIns);
		}
	}
	
	for(int32 index = 0; index < Assets.Num();++index)
	{
		UE_LOG(LogHotPatcherCoreHelper,Display,TEXT("%d packages is cooked,Remain %d Total %d"), index, Assets.Num() - index, Assets.Num());
		CookPackage(Assets[index],CookPlatforms,CookActionCallback,
#if WITH_PACKAGE_CONTEXT
			FinalPlatformSavePackageContext,
#endif
			InSavePath,false);
	}
	// UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
	// UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
}

struct FFilterEditorOnlyFlag
{
	FFilterEditorOnlyFlag(UPackage* InPackage,ITargetPlatform* InPlatform)
	{
		Package = InPackage;
		Platform = InPlatform;
		if(!Platform->HasEditorOnlyData())
		{
			Package->SetPackageFlags(PKG_FilterEditorOnly);
		}
		else
		{
			Package->ClearPackageFlags(PKG_FilterEditorOnly);
		}
	}
	~FFilterEditorOnlyFlag()
	{
		if(!Platform->HasEditorOnlyData())
		{
			Package->ClearPackageFlags(PKG_FilterEditorOnly);
		}
	}
	UPackage* Package;
	ITargetPlatform* Platform;
};
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
UE_TRACE_EVENT_BEGIN(CUSTOM_LOADTIMER_LOG, CookPackage, NoSync)
	UE_TRACE_EVENT_FIELD(Trace::WideString, PackageName)
UE_TRACE_EVENT_END()
#endif

bool UFlibHotPatcherCoreHelper::CookPackage(
	const FSoftObjectPath& AssetObjectPath,
	TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms,
	FCookActionCallback CookActionCallback,
#if WITH_PACKAGE_CONTEXT
	class TMap<FString,FSavePackageContext*> PlatformSavePackageContext,
#endif
	const FString& InSavePath,
	bool bStorageConcurrent 
)
{
	SCOPED_NAMED_EVENT_TEXT("CookPackageForObjectPath",FColor::Red);
	UPackage* Package = UFlibAssetManageHelper::GetPackage(*AssetObjectPath.GetLongPackageName());
	return UFlibHotPatcherCoreHelper::CookPackage(Package,CookPlatforms,CookActionCallback,
#if WITH_PACKAGE_CONTEXT
	PlatformSavePackageContext,
#endif
	InSavePath,bStorageConcurrent);
}


bool UFlibHotPatcherCoreHelper::CookPackage(
	UPackage* Package,
	TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms,
	FCookActionCallback CookActionCallback,
#if WITH_PACKAGE_CONTEXT
	class TMap<FString,FSavePackageContext*> PlatformSavePackageContext,
#endif
	const FString& InSavePath,
	bool bStorageConcurrent
)
{
	FString LongPackageName = UFlibAssetManageHelper::LongPackageNameToPackagePath(Package->GetPathName());
	TMap<FName,FString> PlatformSavePaths;
	for(auto Platform: CookPlatforms)
	{
		FString SavePath = UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(InSavePath,LongPackageName, Platform.Value->PlatformName());
		PlatformSavePaths.Add(*Platform.Value->PlatformName(),SavePath);
	}
	
	return UFlibHotPatcherCoreHelper::CookPackage(Package,CookPlatforms,CookActionCallback,
#if WITH_PACKAGE_CONTEXT
	PlatformSavePackageContext,
#endif
	PlatformSavePaths,bStorageConcurrent);
}

bool UFlibHotPatcherCoreHelper::CookPackage(
	UPackage* Package,
	TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms,
	FCookActionCallback CookActionCallback,
#if WITH_PACKAGE_CONTEXT
	class TMap<FString,FSavePackageContext*> PlatformSavePackageContext,
#endif
	const TMap<FName,FString>& CookedPlatformSavePaths,
	bool bStorageConcurrent
)
{
	bool bSuccessed = false;

	FString LongPackageName = UFlibAssetManageHelper::LongPackageNameToPackagePath(Package->GetPathName());
	FString FakePackageName = FString(TEXT("Package ")) + LongPackageName;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
	SCOPED_CUSTOM_LOADTIMER(CookPackage)
		ADD_CUSTOM_LOADTIMER_META(CookPackage, PackageName, *FakePackageName);
#else
	SCOPED_NAMED_EVENT_TEXT("CookPackage",FColor::Red);
#endif
	{
#if ENGINE_MINOR_VERSION < 26
		FScopedNamedEvent CookPackageEvent(FColor::Red,*FString::Printf(TEXT("%s"),*LongPackageName));
#endif
		// UPackage* Package = UFlibAssetManageHelper::GetPackage(FName(LongPackageName));

		bool bIsFailedPackage = !Package || Package->HasAnyPackageFlags(PKG_EditorOnly);
		if(!Package)
		{
			UE_LOG(LogHotPatcher,Warning,TEXT("Cook %s UPackage is null!"),*LongPackageName);
			return false;
		}
		if(Package && Package->HasAnyPackageFlags(PKG_EditorOnly))
		{
			UE_LOG(LogHotPatcher,Warning,TEXT("Cook %s Failed! It is EditorOnly Package!"),*LongPackageName);
		}

		
		bool bUnversioned = true;
		bool CookLinkerDiff = false;
		uint32 SaveFlags = UFlibHotPatcherCoreHelper::GetCookSaveFlag(Package,bUnversioned,bStorageConcurrent,CookLinkerDiff);
		EObjectFlags CookedFlags = UFlibHotPatcherCoreHelper::GetObjectFlagForCooked(Package);
		
		
	#if ENGINE_MAJOR_VERSION > 4
		FName PackageFileName = Package->GetLoadedPath().GetPackageFName();
	#else
		FName PackageFileName = Package->FileName;
	#endif
		if(PackageFileName.IsNone() && LongPackageName.IsEmpty())
			return bSuccessed;

		uint32 OriginalPackageFlags = Package->GetPackageFlags();
		
		for(auto& Platform:CookPlatforms)
		{
			// if(bIsFailedPackage)
			// {
			// 	CookActionCallback.OnAssetCooked(Package,Platform,ESavePackageResult::Error);
			// 	return false;
			// }
			
			FFilterEditorOnlyFlag SetPackageEditorOnlyFlag(Package,Platform.Value);

			FString PackageName = PackageFileName.IsNone() ? LongPackageName :PackageFileName.ToString();
			FString CookedSavePath = *CookedPlatformSavePaths.Find(*Platform.Value->PlatformName());

			ETargetPlatform TargetPlatform = Platform.Key;

			/* for material cook
			 * Illegal call to StaticFindObject() while serializing object data!
			 * ![](https://img.imzlp.com/imgs/zlp/picgo/2022/202211121451617.png)
			*/
			if(!bStorageConcurrent)
			{
				TSet<UObject*> ProcessedObjs;
				TSet<UObject*> PendingProcessObjs;
				UFlibHotPatcherCoreHelper::CacheForCookedPlatformData(TArray<UPackage*>{Package},TArray<ITargetPlatform*>{Platform.Value},ProcessedObjs,PendingProcessObjs,bStorageConcurrent, false);
			}
			if(GCookLog)
			{
				UE_LOG(LogHotPatcher,Log,TEXT("Cook %s for %s"),*Package->GetName(),*Platform.Value->PlatformName());
			}
	#if WITH_PACKAGE_CONTEXT
			FSavePackageContext* CurrentPlatformPackageContext = nullptr;
			if(PlatformSavePackageContext.Contains(Platform.Value->PlatformName()))
			{
				CurrentPlatformPackageContext = *PlatformSavePackageContext.Find(Platform.Value->PlatformName());
			}
		#if ENGINE_MAJOR_VERSION > 4 /*&& ENGINE_MINOR_VERSION > 0*/
				IPackageWriter::FBeginPackageInfo BeginInfo;
				BeginInfo.PackageName = Package->GetFName();
				BeginInfo.LooseFilePath = CookedSavePath;
				CurrentPlatformPackageContext->PackageWriter->BeginPackage(BeginInfo);
		#endif
	#endif

			if(CookActionCallback.OnCookBegin)
			{
				CookActionCallback.OnCookBegin(PackageName,TargetPlatform);
			}
			
			if (!Platform.Value->HasEditorOnlyData())
			{
				Package->SetPackageFlags(PKG_FilterEditorOnly);
			}
			else
			{
				Package->ClearPackageFlags(PKG_FilterEditorOnly);
			}
			
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			GIsCookerLoadingPackage = true;
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
			FSavePackageResultStruct Result = GEditor->Save(	Package, nullptr, CookedFlags, *CookedSavePath, 
	                                                GError, nullptr, false, false, SaveFlags, Platform.Value, 
	                                                FDateTime::MinValue(), false, /*DiffMap*/ nullptr
	#if WITH_PACKAGE_CONTEXT
	                                                ,CurrentPlatformPackageContext
	#endif
	                                                );
			GIsCookerLoadingPackage = false;
			
			bSuccessed = Result == ESavePackageResult::Success;

			if (CookActionCallback.OnAssetCooked)
			{
				CookActionCallback.OnAssetCooked(Package,TargetPlatform,Result.Result);
			}
			
	#if WITH_PACKAGE_CONTEXT
			// in UE5.1
		#if ENGINE_MAJOR_VERSION > 4 /*&& ENGINE_MINOR_VERSION > 0*/
				// save cooked file to desk in UE5-main
				if(bSuccessed)
				{
					//const FAssetPackageData* AssetPackageData = UFlibAssetManageHelper::GetPackageDataByPackageName(Package->GetFName().ToString());
					ICookedPackageWriter::FCommitPackageInfo Info;
#if UE_VERSION_OLDER_THAN(5,1,0)
					Info.bSucceeded = bSuccessed;
#else
					Info.Status = bSuccessed ? IPackageWriter::ECommitStatus::Success : IPackageWriter::ECommitStatus::Error;
#endif
					Info.PackageName = Package->GetFName();
					// PRAGMA_DISABLE_DEPRECATION_WARNINGS
					Info.PackageGuid = FGuid::NewGuid(); //AssetPackageData ? AssetPackageData->PackageGuid : FGuid::NewGuid();
					// PRAGMA_ENABLE_DEPRECATION_WARNINGS
					// Info.Attachments.Add({ "Dependencies", TargetDomainDependencies });
					// TODO: Reenable BuildDefinitionList once FCbPackage support for empty FCbObjects is in
					//Info.Attachments.Add({ "BuildDefinitionList", BuildDefinitionList });
					Info.WriteOptions = IPackageWriter::EWriteOptions::Write;
					if (!!(SaveFlags & SAVE_ComputeHash))
					{
						Info.WriteOptions |= IPackageWriter::EWriteOptions::ComputeHash;
					}
					CurrentPlatformPackageContext->PackageWriter->CommitPackage(MoveTemp(Info));
				}
		#endif
	#endif
		}

		Package->SetPackageFlagsTo(OriginalPackageFlags);
	}
	return bSuccessed;
}

void UFlibHotPatcherCoreHelper::CookChunkAssets(
	TArray<FAssetDetail> Assets,
	const TArray<ETargetPlatform>& Platforms,
	FCookActionCallback CookActionCallback,
#if WITH_PACKAGE_CONTEXT
	class TMap<ETargetPlatform,FSavePackageContext*> PlatformSavePackageContext,
#endif
	const FString& InSavePath
)
{
	
	TArray<FSoftObjectPath> SoftObjectPaths;

	for(const auto& Asset:Assets)
	{
		SoftObjectPaths.Emplace(Asset.PackagePath.ToString());
	}
	if(!!SoftObjectPaths.Num())
	{
		UFlibHotPatcherCoreHelper::CookAssets(SoftObjectPaths,Platforms,CookActionCallback,
#if WITH_PACKAGE_CONTEXT
		PlatformSavePackageContext,
#endif
		InSavePath);
	}
}

ITargetPlatform* UFlibHotPatcherCoreHelper::GetTargetPlatformByName(const FString& PlatformName)
{
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	ITargetPlatform* PlatformIns = NULL; 
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		if (PlatformName.Equals(TargetPlatform->PlatformName()))
		{
			PlatformIns = TargetPlatform;
		}
	}
	return PlatformIns;
}

TArray<ITargetPlatform*> UFlibHotPatcherCoreHelper::GetTargetPlatformsByNames(const TArray<ETargetPlatform>& Platforms)
{

	TArray<ITargetPlatform*> result;
	for(const auto& Platform:Platforms)
	{
    			
		ITargetPlatform* Found = UFlibHotPatcherCoreHelper::GetTargetPlatformByName(THotPatcherTemplateHelper::GetEnumNameByValue(Platform,false));
		if(Found)
		{
			result.Add(Found);
		}
	}
	return result;
}



FString UFlibHotPatcherCoreHelper::GetUnrealPakBinary()
{
#if PLATFORM_WINDOWS
	return FPaths::Combine(
        FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
        TEXT("Binaries"),
#if PLATFORM_64BITS	
        TEXT("Win64"),
#else
        TEXT("Win32"),
#endif
        TEXT("UnrealPak.exe")
    );
#endif

#if PLATFORM_MAC
	return FPaths::Combine(
            FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
            TEXT("Binaries"),
            TEXT("Mac"),
            TEXT("UnrealPak")
    );
#endif

	return TEXT("");
}

FString UFlibHotPatcherCoreHelper::GetUECmdBinary()
{
	FString Binary;
#if ENGINE_MAJOR_VERSION > 4
	Binary = TEXT("UnrealEditor");
#else
	Binary = TEXT("UE4Editor");
#endif

	FString ConfigutationName = ANSI_TO_TCHAR(COMPILER_CONFIGURATION_NAME);
	bool bIsDevelopment = ConfigutationName.Equals(TEXT("Development"));
	
#if PLATFORM_WINDOWS
	FString PlatformName;
	#if PLATFORM_64BITS	
		PlatformName = TEXT("Win64");
	#else
		PlatformName = TEXT("Win32");
	#endif

	return FPaths::Combine(
        FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
        TEXT("Binaries"),PlatformName,FString::Printf(TEXT("%s%s-Cmd.exe"),*Binary,bIsDevelopment ? TEXT("") : *FString::Printf(TEXT("-%s-%s"),*PlatformName,*ConfigutationName)));
#endif
	
#if PLATFORM_MAC
#if ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION <= 21
	return FPaths::Combine(
			FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
			TEXT("Binaries"),TEXT("Mac"),TEXT("UE4Editor.app/Contents/MacOS"),
			FString::Printf(TEXT("%s%s"),*Binary,
				bIsDevelopment ? TEXT("") : *FString::Printf(TEXT("-Mac-%s"),*ConfigutationName)));
#else
	return FPaths::Combine(
			FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
			TEXT("Binaries"),TEXT("Mac"),
			FString::Printf(TEXT("%s%s-Cmd"),*Binary,
				bIsDevelopment ? TEXT("") : *FString::Printf(TEXT("-Mac-%s"),*ConfigutationName)));
#endif
#endif
	return TEXT("");
}


FProcHandle UFlibHotPatcherCoreHelper::DoUnrealPak(TArray<FString> UnrealPakCommandletOptions, bool block)
{
	FString UnrealPakBinary = UFlibHotPatcherCoreHelper::GetUnrealPakBinary();

	FString CommandLine;
	for (const auto& Option : UnrealPakCommandletOptions)
	{
		CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
	}

	// create UnrealPak process

	uint32 *ProcessID = NULL;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*UnrealPakBinary, *CommandLine, true, false, false, ProcessID, 1, NULL, NULL, NULL);

	if (ProcHandle.IsValid())
	{
		if (block)
		{
			FPlatformProcess::WaitForProc(ProcHandle);
		}
	}
	return ProcHandle;
}

FString UFlibHotPatcherCoreHelper::GetMetadataDir(const FString& ProjectDir, const FString& ProjectName,ETargetPlatform Platform)
{
	FString result;
	FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform,false);
	return FPaths::Combine(ProjectDir,TEXT("Saved/Cooked"),PlatformName,ProjectName,TEXT("Metadata"));
}

void UFlibHotPatcherCoreHelper::CleanDefaultMetadataCache(const TArray<ETargetPlatform>& TargetPlatforms)
{
	SCOPED_NAMED_EVENT_TEXT("CleanDefaultMetadataCache",FColor::Red);
	for(ETargetPlatform Platform:TargetPlatforms)
	{
		FString MetadataDir = GetMetadataDir(FPaths::ProjectDir(),FApp::GetProjectName(),Platform);
		if(FPaths::DirectoryExists(MetadataDir))
		{
			IFileManager::Get().DeleteDirectory(*MetadataDir,false,true);
		}
	}
}

void UFlibHotPatcherCoreHelper::BackupMetadataDir(const FString& ProjectDir, const FString& ProjectName,
                                                  const TArray<ETargetPlatform>& Platforms, const FString& OutDir)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	for(const auto& Platform:Platforms)
	{
		FString MetadataDir = FPaths::ConvertRelativePathToFull(UFlibHotPatcherCoreHelper::GetMetadataDir(ProjectDir,ProjectName,Platform));
		FString OutMetadir = FPaths::Combine(OutDir,TEXT("BackupMetadatas"),THotPatcherTemplateHelper::GetEnumNameByValue(Platform,false));
		if(FPaths::DirectoryExists(MetadataDir))
		{
			PlatformFile.CreateDirectoryTree(*OutMetadir);
			PlatformFile.CopyDirectoryTree(*OutMetadir,*MetadataDir,true);
		}
	}
}

void UFlibHotPatcherCoreHelper::BackupProjectConfigDir(const FString& ProjectDir,const FString& OutDir)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString ConfigDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	FString OutConfigdir = FPaths::Combine(OutDir,TEXT("Config"));
	if(FPaths::DirectoryExists(ConfigDir))
	{
		PlatformFile.CreateDirectoryTree(*OutConfigdir);
		PlatformFile.CopyDirectoryTree(*OutConfigdir,*ConfigDir,true);
	}
}

FString UFlibHotPatcherCoreHelper::ReleaseSummary(const FHotPatcherVersion& NewVersion)
{
	FString result = TEXT("\n---------------------HotPatcher Release Summary---------------------\n");

	auto ParserPlatformExternAssets = [](ETargetPlatform Platform,const FPlatformExternAssets& PlatformExternAssets)->FString
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);;
		FString result = FString::Printf(TEXT("======== %s Non-Assets =========\n"),*PlatformName);
		uint32 BeginLength = result.Len();
		result += FString::Printf(TEXT("External Files: %d\n"),PlatformExternAssets.AddExternFileToPak.Num());
		// result += FString::Printf(TEXT("External Directorys: %d\n"),PlatformExternAssets.AddExternDirectoryToPak.Num());

		for(uint32 index=0;index<BeginLength;++index)
			result += TEXT("=");
		result+=TEXT("\n");
		return result;
	};
	TMap<FString,uint32> AddModuleAssetNumMap;
	result += FString::Printf(TEXT("New Release Asset Number: %d\n"),UFlibAssetManageHelper::ParserAssetDependenciesInfoNumber(NewVersion.AssetInfo,AddModuleAssetNumMap));
	result += UFlibAssetManageHelper::ParserModuleAssetsNumMap(AddModuleAssetNumMap);
	TArray<ETargetPlatform> Keys;
	NewVersion.PlatformAssets.GetKeys(Keys);
	for(const auto& Key:Keys)
	{
		result += ParserPlatformExternAssets(Key,*NewVersion.PlatformAssets.Find(Key));
	}
	return result;
}

FString UFlibHotPatcherCoreHelper::PatchSummary(const FPatchVersionDiff& DiffInfo)
{
	auto ExternFileSummary = [](ETargetPlatform Platform,const FPatchVersionExternDiff& ExternDiff)->FString
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);;
		FString result = FString::Printf(TEXT("======== %s Non-Assets =========\n"),*PlatformName);
		uint32 BeginLength = result.Len();
		result += FString::Printf(TEXT("Add Non-Asset Number: %d\n"),ExternDiff.AddExternalFiles.Num());
		result += FString::Printf(TEXT("Modify Non-Asset Number: %d\n"),ExternDiff.ModifyExternalFiles.Num());
		result += FString::Printf(TEXT("Delete Non-Asset Number: %d\n"),ExternDiff.DeleteExternalFiles.Num());
		for(uint32 index=0;index<BeginLength;++index)
			result += TEXT("=");
		result+=TEXT("\n");
		return result;
	};
	
	FString result = TEXT("\n-----------------HotPatcher Patch Summary-----------------\n");
	result+= TEXT("=================\n");
	TMap<FString,uint32> AddModuleAssetNumMap;
	result += FString::Printf(TEXT("Add Asset Number: %d\n"),UFlibAssetManageHelper::ParserAssetDependenciesInfoNumber(DiffInfo.AssetDiffInfo.AddAssetDependInfo,AddModuleAssetNumMap));
	result += UFlibAssetManageHelper::ParserModuleAssetsNumMap(AddModuleAssetNumMap);
	TMap<FString,uint32> ModifyModuleAssetNumMap;
	result += FString::Printf(TEXT("Modify Asset Number: %d\n"),UFlibAssetManageHelper::ParserAssetDependenciesInfoNumber(DiffInfo.AssetDiffInfo.ModifyAssetDependInfo,ModifyModuleAssetNumMap));
	result += UFlibAssetManageHelper::ParserModuleAssetsNumMap(ModifyModuleAssetNumMap);
	TMap<FString,uint32> DeleteModuleAssetNumMap;
	result += FString::Printf(TEXT("Delete Asset Number: %d\n"),UFlibAssetManageHelper::ParserAssetDependenciesInfoNumber(DiffInfo.AssetDiffInfo.DeleteAssetDependInfo,DeleteModuleAssetNumMap));
	result += UFlibAssetManageHelper::ParserModuleAssetsNumMap(DeleteModuleAssetNumMap);
	TArray<ETargetPlatform> Platforms;
	DiffInfo.PlatformExternDiffInfo.GetKeys(Platforms);
	for(const auto& Platform:Platforms)
	{
		result += ExternFileSummary(Platform,*DiffInfo.PlatformExternDiffInfo.Find(Platform));
	}
	return result;
}

FString UFlibHotPatcherCoreHelper::ReplacePakRegular(const FReplacePakRegular& RegularConf, const FString& InRegular)
{
	return UFlibPatchParserHelper::ReplacePakRegular(RegularConf,InRegular);
}

bool UFlibHotPatcherCoreHelper::CheckSelectedAssetsCookStatus(const FString& OverrideCookedDir,const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg)
{
	OutMsg.Empty();
	// 检查所修改的资源是否被Cook过
	for (const auto& PlatformName : PlatformNames)
	{
		TArray<FAssetDetail> ValidCookAssets;
		TArray<FAssetDetail> InvalidCookAssets;

		UFlibHotPatcherCoreHelper::CheckInvalidCookFilesByAssetDependenciesInfo(UKismetSystemLibrary::GetProjectDirectory(),OverrideCookedDir, PlatformName, SelectedAssets, ValidCookAssets, InvalidCookAssets);

		if (InvalidCookAssets.Num() > 0)
		{
			OutMsg.Append(FString::Printf(TEXT("%s UnCooked Assets:\n"), *PlatformName));

			for (const auto& Asset : InvalidCookAssets)
			{
				FString AssetLongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(Asset.PackagePath.ToString());
				OutMsg.Append(FString::Printf(TEXT("\t%s\n"), *AssetLongPackageName));
			}
		}
	}

	return OutMsg.IsEmpty();
}

bool UFlibHotPatcherCoreHelper::CheckPatchRequire(const FString& OverrideCookedDir,const FPatchVersionDiff& InDiff,const TArray<FString>& PlatformNames,FString& OutMsg)
{
	bool Status = false;
	// 错误处理
	{
		FString GenErrorMsg;
		FAssetDependenciesInfo AllChangedAssetInfo = UFlibAssetManageHelper::CombineAssetDependencies(InDiff.AssetDiffInfo.AddAssetDependInfo, InDiff.AssetDiffInfo.ModifyAssetDependInfo);
		bool bSelectedCookStatus = CheckSelectedAssetsCookStatus(OverrideCookedDir,PlatformNames, AllChangedAssetInfo, GenErrorMsg);

		// 如果有错误信息 则输出后退出
		if (!bSelectedCookStatus)
		{
			OutMsg = GenErrorMsg;
			Status = false;
		}
		else
		{
			OutMsg = TEXT("");
			Status = true;
		}
	}
	return Status;
}

FString UFlibHotPatcherCoreHelper::Conv2IniPlatform(const FString& Platform)
{
	FString Result;
	static TMap<FString,FString> PlatformMaps;
	static bool bInit = false;
	if(!bInit)
	{
		ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
		const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
		TArray<ITargetPlatform*> CookPlatforms; 
		for (ITargetPlatform *TargetPlatform : TargetPlatforms)
		{
			PlatformMaps.Add(TargetPlatform->PlatformName(),TargetPlatform->IniPlatformName());
		}
		bInit = true;
	}
	
	if(PlatformMaps.Contains(Platform))
	{
		Result = *PlatformMaps.Find(Platform);
	}
	return Result;
}

TArray<FString> UFlibHotPatcherCoreHelper::GetSupportPlatforms()
{
	TArray<FString> Result;
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	TArray<ITargetPlatform*> CookPlatforms; 
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		Result.AddUnique(TargetPlatform->PlatformName());
	}
	return Result;
}

#define ENCRYPT_CRYPTO_NAME TEXT("cryptokeys")

FString UFlibHotPatcherCoreHelper::GetEncryptSettingsCommandlineOptions(const FPakEncryptSettings& PakEncryptSettings,const FString& PlatformName)
{
	FString Result; 

	FString CryptoKey = UFlibPatchParserHelper::ReplaceMarkPath(PakEncryptSettings.CryptoKeys.FilePath);

	auto AppendCommandOptions = [&Result](bool bEncryptIndex,bool bbEncrypt,bool bSign)
	{
		if(bbEncrypt)
		{
			Result += TEXT("-encrypt ");
		}
		if(bEncryptIndex)
		{
			Result += TEXT("-encryptindex ");
		}
		if(bSign)
		{
			Result += TEXT("-sign ");
		}
	};
	
	FEncryptSetting EncryptSettings = UFlibPatchParserHelper::GetCryptoSettingByPakEncryptSettings(PakEncryptSettings);
	if(PakEncryptSettings.bUseDefaultCryptoIni)
	{
		FString SaveTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher"),TEXT("Crypto.json")));
		FPakEncryptionKeys PakEncryptionKeys = UFlibPatchParserHelper::GetCryptoByProjectSettings();
		UFlibPatchParserHelper::SerializePakEncryptionKeyToFile(PakEncryptionKeys,SaveTo);
		CryptoKey = SaveTo;
	}
	
	AppendCommandOptions(
		EncryptSettings.bEncryptIndex,
		(EncryptSettings.bEncryptIniFiles || EncryptSettings.bEncryptAllAssetFiles || EncryptSettings.bEncryptUAssetFiles),
		EncryptSettings.bSign
		);
	
	if(FPaths::FileExists(CryptoKey))
	{
		Result += FString::Printf(TEXT("-%s=\"%s\" "),ENCRYPT_CRYPTO_NAME,*CryptoKey);
	}
	
	Result += FString::Printf(TEXT("-projectdir=\"%s\" "),*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
	Result += FString::Printf(TEXT("-enginedir=\"%s\" "),*FPaths::ConvertRelativePathToFull(FPaths::EngineDir()));
	Result += FString::Printf(TEXT("-platform=%s"),*PlatformName);
	return Result;
}

ITargetPlatform* UFlibHotPatcherCoreHelper::GetPlatformByName(const FString& Name)
{
	static TMap<FString,ITargetPlatform*> PlatformNameMap;

	if(PlatformNameMap.Contains(Name))
		return *PlatformNameMap.Find(Name);
	
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	ITargetPlatform* result = NULL;
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		if (Name.Equals(TargetPlatform->PlatformName()))
		{
			result = TargetPlatform;
			PlatformNameMap.Add(Name,TargetPlatform);
			break;
		}
	}
	return result;
}

#include "BaseWidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"

FPatchVersionDiff UFlibHotPatcherCoreHelper::DiffPatchVersionWithPatchSetting(const FExportPatchSettings& PatchSetting, const FHotPatcherVersion& Base, const FHotPatcherVersion& New)
{
	SCOPED_NAMED_EVENT_TEXT("DiffPatchVersionWithPatchSetting",FColor::Red);
	FPatchVersionDiff VersionDiffInfo;
	const FAssetDependenciesInfo& BaseVersionAssetDependInfo = Base.AssetInfo;
	const FAssetDependenciesInfo& CurrentVersionAssetDependInfo = New.AssetInfo;

	UFlibPatchParserHelper::DiffVersionAssets(
		CurrentVersionAssetDependInfo,
		BaseVersionAssetDependInfo,
		VersionDiffInfo.AssetDiffInfo.AddAssetDependInfo,
		VersionDiffInfo.AssetDiffInfo.ModifyAssetDependInfo,
		VersionDiffInfo.AssetDiffInfo.DeleteAssetDependInfo
	);

	UFlibPatchParserHelper::DiffVersionAllPlatformExFiles(PatchSetting,Base,New,VersionDiffInfo.PlatformExternDiffInfo);

	if(PatchSetting.GetIgnoreDeletionModulesAsset().Num())
	{
		for(const auto& ModuleName:PatchSetting.GetIgnoreDeletionModulesAsset())
		{
			VersionDiffInfo.AssetDiffInfo.DeleteAssetDependInfo.AssetsDependenciesMap.Remove(ModuleName);
		}
	}
	
	if(PatchSetting.IsRecursiveWidgetTree())
	{
		AnalysisWidgetTree(Base,New,VersionDiffInfo);
	}
	if(PatchSetting.IsAnalysisMatInstance())
	{
		AnalysisMaterialInstance(Base,New,VersionDiffInfo);
	}
	
	if(PatchSetting.IsForceSkipContent())
	{
		TArray<FString> AllSkipDirContents = UFlibAssetManageHelper::DirectoriesToStrings(PatchSetting.GetForceSkipContentRules());
		UFlibPatchParserHelper::ExcludeContentForVersionDiff(VersionDiffInfo,AllSkipDirContents,EHotPatcherMatchModEx::StartWith);
		TArray<FString> AllSkipAssets = UFlibAssetManageHelper::SoftObjectPathsToStrings(PatchSetting.GetForceSkipAssets());
		UFlibPatchParserHelper::ExcludeContentForVersionDiff(VersionDiffInfo,AllSkipAssets,EHotPatcherMatchModEx::Equal);
	}
	// clean deleted asset info in patch
	if(PatchSetting.IsIgnoreDeletedAssetsInfo())
	{
		UE_LOG(LogHotPatcher,Display,TEXT("ignore deleted assets info in patch..."));
		VersionDiffInfo.AssetDiffInfo.DeleteAssetDependInfo.AssetsDependenciesMap.Empty();
		if(VersionDiffInfo.PlatformExternDiffInfo.Contains(ETargetPlatform::AllPlatforms))
		{
			VersionDiffInfo.PlatformExternDiffInfo.Find(ETargetPlatform::AllPlatforms)->DeleteExternalFiles.Empty();
		}
		for(const auto& Platform:PatchSetting.GetPakTargetPlatforms())
		{
			if (auto PlatExtDiffInfo = VersionDiffInfo.PlatformExternDiffInfo.Find(Platform))
			{
				PlatExtDiffInfo->DeleteExternalFiles.Empty();
			}
		}
	}
	
	return VersionDiffInfo;
}


void UFlibHotPatcherCoreHelper::AnalysisWidgetTree(const FHotPatcherVersion& Base, const FHotPatcherVersion& New,FPatchVersionDiff& PakDiff,int32 flags)
{
	UClass* WidgetBlueprintClass = UWidgetBlueprint::StaticClass();
	UFlibHotPatcherCoreHelper::AnalysisDependenciesAssets(Base,New,PakDiff,WidgetBlueprintClass,TArray<UClass*>{WidgetBlueprintClass},true);
}

void UFlibHotPatcherCoreHelper::AnalysisMaterialInstance(const FHotPatcherVersion& Base, const FHotPatcherVersion& New,FPatchVersionDiff& PakDiff,int32 flags)
{
	UClass* UMaterialClass = UMaterial::StaticClass();
	TArray<UClass*> MaterialInstanceClasses = GetDerivedClasses(UMaterialInstance::StaticClass(),true,true);
	UFlibHotPatcherCoreHelper::AnalysisDependenciesAssets(Base,New,PakDiff,UMaterialClass,MaterialInstanceClasses,false);
}

void UFlibHotPatcherCoreHelper::AnalysisDependenciesAssets(const FHotPatcherVersion& Base, const FHotPatcherVersion& New,FPatchVersionDiff& PakDiff,UClass* SearchClass,TArray<UClass*> DependenciesClass,bool bRecursive,int32 flags)
{
	TArray<FAssetDetail> AnalysisAssets;
	if(flags & 0x1)
	{
		const TArray<FAssetDetail>& AddAssets = PakDiff.AssetDiffInfo.AddAssetDependInfo.GetAssetDetails();
		AnalysisAssets.Append(AddAssets);
		
	}
	if(flags & 0x2)
	{
		const TArray<FAssetDetail>& ModifyAssets = PakDiff.AssetDiffInfo.ModifyAssetDependInfo.GetAssetDetails();
		AnalysisAssets.Append(ModifyAssets);
	}
	TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDepTypes {EAssetRegistryDependencyTypeEx::Hard};

	FName ClassName = SearchClass->GetFName();;
	
	for(const auto& OriginAsset:AnalysisAssets)
	{
		if(OriginAsset.AssetType.IsEqual(ClassName))
		{
			for(auto& DepencenciesClassItem:DependenciesClass)
			{
				FName DependenciesName = DepencenciesClassItem->GetFName();
				TArray<FAssetDetail> RefAssets = UFlibHotPatcherCoreHelper::GetReferenceRecursivelyByClassName(OriginAsset,TArray<FString>{DependenciesName.ToString()},AssetRegistryDepTypes,bRecursive);
				for(const auto& Asset:RefAssets)
				{
					if(!(Asset.AssetType.IsEqual(DependenciesName)))
					{
						continue;
					}
					FString PackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(Asset.PackagePath.ToString());
					bool bInBaseVersion = Base.AssetInfo.HasAsset(PackageName);
					if(bInBaseVersion)
					{
						PakDiff.AssetDiffInfo.ModifyAssetDependInfo.AddAssetsDetail(Asset);
						UE_LOG(LogHotPatcher,Log,TEXT("Add Parent: %s"),*Asset.PackagePath.ToString());
					}
				}
			}
		}
	}
}

TArray<FAssetDetail> UFlibHotPatcherCoreHelper::GetReferenceRecursivelyByClassName(const FAssetDetail& AssetDetail,const TArray<FString>& AssetTypeNames,const TArray<EAssetRegistryDependencyTypeEx>& RefType,bool bRecursive)
{
	TArray<FAssetDetail> Results;
	
	TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDepTypes {EAssetRegistryDependencyTypeEx::Hard};
	TArray<EAssetRegistryDependencyType::Type> SearchTypes;
	for(auto TypeEx:AssetRegistryDepTypes)
	{
		SearchTypes.AddUnique(UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(TypeEx));
	}
	
	TArray<FAssetDetail> CurrentAssetsRef;
	UFlibAssetManageHelper::GetAssetReferenceRecursively(AssetDetail, SearchTypes, AssetTypeNames, CurrentAssetsRef,bRecursive);
	for(const auto& Asset:CurrentAssetsRef)
	{
		if(!AssetTypeNames.Contains(Asset.AssetType.ToString()))
		{
			continue;
		}
		Results.AddUnique(Asset);
	}
	
	return Results;
}

TArray<UClass*> UFlibHotPatcherCoreHelper::GetDerivedClasses(UClass* BaseClass,bool bRecursive, bool bContainSelf)
{
	TArray<UClass*> Classes;
	if(bContainSelf)
	{
		Classes.AddUnique(BaseClass);
	}
	
	TArray<UClass*> AllDerivedClass;
	::GetDerivedClasses(BaseClass,AllDerivedClass,bRecursive);
	for(auto classIns:AllDerivedClass)
	{
		Classes.AddUnique(classIns);
	}
	return Classes;
}

void UFlibHotPatcherCoreHelper::DeleteDirectory(const FString& Dir)
{
	if(!Dir.IsEmpty() && FPaths::DirectoryExists(Dir))
	{
		UE_LOG(LogHotPatcher,Display,TEXT("delete dir %s"),*Dir);
		IFileManager::Get().DeleteDirectory(*Dir,true,true);
	}
}

int32 UFlibHotPatcherCoreHelper::GetMemoryMappingAlignment(const FString& PlatformName)
{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 22
	int32 result = 0;
	
	ITargetPlatform* Platform =  UFlibHotPatcherCoreHelper::GetPlatformByName(PlatformName);
	if(Platform)
	{
		result = Platform->GetMemoryMappingAlignment();
	}
	return result;
#else
	if(PlatformName.Equals(TEXT("IOS")))
	{
		return 16384;
	}
	return 0;
#endif
}

FChunkAssetDescribe UFlibHotPatcherCoreHelper::DiffChunkWithPatchSetting(
	const FExportPatchSettings& PatchSetting,
	const FChunkInfo& CurrentVersionChunk,
	const FChunkInfo& TotalChunk
	//,TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	FHotPatcherVersion TotalChunkVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT(""),
		TEXT(""),
		TEXT(""),
		TotalChunk,
		//ScanedCaches,
		PatchSetting.IsIncludeHasRefAssetsOnly(),
		TotalChunk.bAnalysisFilterDependencies,
		PatchSetting.GetHashCalculator()
	);

	return UFlibHotPatcherCoreHelper::DiffChunkByBaseVersionWithPatchSetting(PatchSetting, CurrentVersionChunk ,TotalChunk, TotalChunkVersion/*,ScanedCaches*/);
}

FChunkAssetDescribe UFlibHotPatcherCoreHelper::DiffChunkByBaseVersionWithPatchSetting(
	const FExportPatchSettings& PatchSetting,
	const FChunkInfo& CurrentVersionChunk,
	const FChunkInfo& TotalChunk,
	const FHotPatcherVersion& BaseVersion
	//,TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	FChunkAssetDescribe result;
	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT(""),
		TEXT(""),
		TEXT(""),
		CurrentVersionChunk,
		//ScanedCaches,
		PatchSetting.IsIncludeHasRefAssetsOnly(),
		CurrentVersionChunk.bAnalysisFilterDependencies,
		PatchSetting.GetHashCalculator()
	);
	FPatchVersionDiff ChunkDiffInfo = UFlibHotPatcherCoreHelper::DiffPatchVersionWithPatchSetting(PatchSetting, BaseVersion, CurrentVersion);
	
	result.Assets = UFlibAssetManageHelper::CombineAssetDependencies(ChunkDiffInfo.AssetDiffInfo.AddAssetDependInfo, ChunkDiffInfo.AssetDiffInfo.ModifyAssetDependInfo);

	TArray<ETargetPlatform> Platforms;
	ChunkDiffInfo.PlatformExternDiffInfo.GetKeys(Platforms);
	for(auto Platform:Platforms)
	{
		FPlatformExternFiles PlatformFiles;
		PlatformFiles.Platform = Platform;
		PlatformFiles.ExternFiles = ChunkDiffInfo.PlatformExternDiffInfo.Find(Platform)->AddExternalFiles;
		PlatformFiles.ExternFiles.Append(ChunkDiffInfo.PlatformExternDiffInfo.Find(Platform)->ModifyExternalFiles);
		result.AllPlatformExFiles.Add(Platform,PlatformFiles);	
	}
	
	result.InternalFiles.bIncludeAssetRegistry = CurrentVersionChunk.InternalFiles.bIncludeAssetRegistry != TotalChunk.InternalFiles.bIncludeAssetRegistry;
	result.InternalFiles.bIncludeGlobalShaderCache = CurrentVersionChunk.InternalFiles.bIncludeGlobalShaderCache != TotalChunk.InternalFiles.bIncludeGlobalShaderCache;
	result.InternalFiles.bIncludeShaderBytecode = CurrentVersionChunk.InternalFiles.bIncludeShaderBytecode != TotalChunk.InternalFiles.bIncludeShaderBytecode;
	result.InternalFiles.bIncludeEngineIni = CurrentVersionChunk.InternalFiles.bIncludeEngineIni != TotalChunk.InternalFiles.bIncludeEngineIni;
	result.InternalFiles.bIncludePluginIni = CurrentVersionChunk.InternalFiles.bIncludePluginIni != TotalChunk.InternalFiles.bIncludePluginIni;
	result.InternalFiles.bIncludeProjectIni = CurrentVersionChunk.InternalFiles.bIncludeProjectIni != TotalChunk.InternalFiles.bIncludeProjectIni;

	return result;
}


bool UFlibHotPatcherCoreHelper::SerializeAssetRegistryByDetails(IAssetRegistry* AssetRegistry,
                                                                const FString& PlatformName, const TArray<FAssetDetail>& AssetDetails, const FString& SavePath)
{
	
	ITargetPlatform* TargetPlatform =  UFlibHotPatcherCoreHelper::GetPlatformByName(PlatformName);
	FAssetRegistrySerializationOptions SaveOptions;
	AssetRegistry->InitializeSerializationOptions(SaveOptions, TargetPlatform->IniPlatformName());
	SaveOptions.bSerializeAssetRegistry = true;
	
	return UFlibHotPatcherCoreHelper::SerializeAssetRegistryByDetails(AssetRegistry,PlatformName,AssetDetails,SavePath, SaveOptions);
}

bool UFlibHotPatcherCoreHelper::SerializeAssetRegistryByDetails(IAssetRegistry* AssetRegistry,
                                                                const FString& PlatformName, const TArray<FAssetDetail>& AssetDetails, const FString& SavePath, FAssetRegistrySerializationOptions SaveOptions)
{
	TArray<FString> PackagePaths;

	for(const auto& Detail:AssetDetails)
	{
		PackagePaths.AddUnique(Detail.PackagePath.ToString());
	}

	return UFlibHotPatcherCoreHelper::SerializeAssetRegistry(AssetRegistry,PlatformName,PackagePaths,SavePath, SaveOptions);
}

bool UFlibHotPatcherCoreHelper::SerializeAssetRegistry(IAssetRegistry* AssetRegistry,
                                                       const FString& PlatformName, const TArray<FString>& PackagePaths, const FString& SavePath, FAssetRegistrySerializationOptions SaveOptions)
{
	
	FAssetRegistryState State;
	// FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	// IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry->Tick(-1.0f);
	AssetRegistry->InitializeTemporaryAssetRegistryState(State, SaveOptions, true);
	for(const auto& AssetPackagePath:PackagePaths)
	{
		if (State.GetAssetByObjectPath(FName(*AssetPackagePath)))
		{
			UE_LOG(LogHotPatcherCoreHelper, Warning, TEXT("%s already add to AssetRegistryState!"), *AssetPackagePath);
			continue;
		}
		FAssetData* AssetData = new FAssetData();
		if (UFlibAssetManageHelper::GetSingleAssetsData(AssetPackagePath, *AssetData))
		{
			if (AssetPackagePath != AssetData->ObjectPath.ToString())
			{
				UE_LOG(LogHotPatcherCoreHelper, Warning, TEXT("%s is a redirector of %s, skip!"), *AssetPackagePath, *AssetData->ObjectPath.ToString());
				delete AssetData;
				continue;
			}
			State.AddAssetData(AssetData);
			continue;
		}
		delete AssetData;
	}
	// Create runtime registry data
	FArrayWriter SerializedAssetRegistry;
	SerializedAssetRegistry.SetFilterEditorOnly(true);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	bool bStateSave = State.Save(SerializedAssetRegistry, SaveOptions);
#else
	bool bStateSave = State.Serialize(SerializedAssetRegistry, SaveOptions);
#endif
	bool result = false;
	// Save the generated registry
	if(bStateSave && FFileHelper::SaveArrayToFile(SerializedAssetRegistry, *SavePath))
	{
		result = true;
		UE_LOG(LogHotPatcher,Log,TEXT("Serialize %s AssetRegistry"),*SavePath);
	}
	return result;
}


FHotPatcherVersion UFlibHotPatcherCoreHelper::MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion, FExportPatchSettings* InPatchSettings)
{
	FHotPatcherVersion BaseVersion = InBaseVersion;
	
	FPatchVersionDiff DiffInfo = UFlibHotPatcherCoreHelper::DiffPatchVersionWithPatchSetting(*InPatchSettings,BaseVersion, InCurrentVersion);
	return UFlibHotPatcherCoreHelper::MakeNewReleaseByDiff(InBaseVersion,DiffInfo, InPatchSettings);
}

FHotPatcherVersion UFlibHotPatcherCoreHelper::MakeNewReleaseByDiff(const FHotPatcherVersion& InBaseVersion,
	const FPatchVersionDiff& InDiff, FExportPatchSettings* InPatchSettings)
{
	FHotPatcherVersion BaseVersion = InBaseVersion;
	FHotPatcherVersion NewRelease;

	NewRelease.BaseVersionId = InBaseVersion.VersionId;
	NewRelease.Date = FDateTime::UtcNow().ToString();
	NewRelease.VersionId = InPatchSettings ? InPatchSettings->VersionId : TEXT("");
	
	FAssetDependenciesInfo& BaseAssetInfoRef = BaseVersion.AssetInfo;
	// TMap<FString, FExternFileInfo>& BaseExternalFilesRef = BaseVersion.ExternalFiles;
	TMap<ETargetPlatform,FPlatformExternAssets>& BasePlatformAssetsRef = BaseVersion.PlatformAssets;

	// Modify Asset
	auto DeleteOldAssetLambda = [&BaseAssetInfoRef](const FAssetDependenciesInfo& InAssetDependenciesInfo)
	{
		for (const auto& AssetsModulePair : InAssetDependenciesInfo.AssetsDependenciesMap)
		{
			FAssetDependenciesDetail* NewReleaseModuleAssets = BaseAssetInfoRef.AssetsDependenciesMap.Find(AssetsModulePair.Key);

			for (const auto& NeedDeleteAsset : AssetsModulePair.Value.AssetDependencyDetails)
			{
				if (NewReleaseModuleAssets && NewReleaseModuleAssets->AssetDependencyDetails.Contains(NeedDeleteAsset.Key))
				{
					NewReleaseModuleAssets->AssetDependencyDetails.Remove(NeedDeleteAsset.Key);
				}
			}
		}
	};
	
	DeleteOldAssetLambda(InDiff.AssetDiffInfo.ModifyAssetDependInfo);
	if(InPatchSettings && !InPatchSettings->IsSaveDeletedAssetsToNewReleaseJson())
	{
		DeleteOldAssetLambda(InDiff.AssetDiffInfo.DeleteAssetDependInfo);
	}

	// Add Asset
	BaseAssetInfoRef = UFlibAssetManageHelper::CombineAssetDependencies(BaseAssetInfoRef, InDiff.AssetDiffInfo.AddAssetDependInfo);
	// modify Asset
	BaseAssetInfoRef = UFlibAssetManageHelper::CombineAssetDependencies(BaseAssetInfoRef, InDiff.AssetDiffInfo.ModifyAssetDependInfo);
	NewRelease.AssetInfo = BaseAssetInfoRef;

	// // external files
	// auto RemoveOldExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternFileInfo>& InFiles)
	// {
	// 	for (const auto& File : InFiles)
	// 	{
	// 		if (BaseExternalFilesRef.Contains(File.FilePath.FilePath))
	// 		{
	// 			BaseExternalFilesRef.Remove(File.FilePath.FilePath);
	// 		}
	// 	}
	// };

	TArray<ETargetPlatform> DiffPlatforms;
	InDiff.PlatformExternDiffInfo.GetKeys(DiffPlatforms);

	for(auto Platform:DiffPlatforms)
	{
		FPlatformExternAssets AddPlatformFiles;
		AddPlatformFiles.TargetPlatform = Platform;
		AddPlatformFiles.AddExternFileToPak = InDiff.PlatformExternDiffInfo[Platform].AddExternalFiles;
		AddPlatformFiles.AddExternFileToPak.Append(InDiff.PlatformExternDiffInfo[Platform].ModifyExternalFiles);
		if(BasePlatformAssetsRef.Contains(Platform))
		{
			for(const auto& File:AddPlatformFiles.AddExternFileToPak)
			{
				if(BasePlatformAssetsRef[Platform].AddExternFileToPak.Contains(File))
				{
					BasePlatformAssetsRef[Platform].AddExternFileToPak.Remove(File);
				}
				BasePlatformAssetsRef[Platform].AddExternFileToPak.Add(File);
			}
		}else
		{
			BasePlatformAssetsRef.Add(Platform,AddPlatformFiles);
		}
	}
	// RemoveOldExternalFilesLambda(DiffInfo.ExternDiffInfo.ModifyExternalFiles);
	// DeleteOldExternalFilesLambda(DiffInfo.DeleteExternalFiles);

	NewRelease.PlatformAssets = BasePlatformAssetsRef;
	return NewRelease;
}


FString UFlibHotPatcherCoreHelper::GetPakCommandExtersion(const FString& InCommand)
{
	auto GetPakCommandFileExtensionLambda = [](const FString& Command)->FString
	{
		FString result;
		int32 DotPos = Command.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (DotPos != INDEX_NONE)
		{
			result =  Command.Mid(DotPos + 1);
			int32 FirstDoubleQuotesPos = -1;
			if(result.FindChar('"',FirstDoubleQuotesPos))
			{
				result.RemoveAt(FirstDoubleQuotesPos,result.Len()-FirstDoubleQuotesPos);
			}
		}
		return result;
	};
	return GetPakCommandFileExtensionLambda(InCommand);
}

TArray<FString> UFlibHotPatcherCoreHelper::GetExtensionsToNotUsePluginCompressionByGConfig()
{
	TArray<FString> IgnoreCompressFormats;
	GConfig->GetArray(TEXT("Pak"),TEXT("ExtensionsToNotUsePluginCompression"),IgnoreCompressFormats,GEngineIni);
	return IgnoreCompressFormats;
}

void UFlibHotPatcherCoreHelper::AppendPakCommandOptions(TArray<FString>& OriginCommands,
	const TArray<FString>& Options, bool bAppendAllMatch, const TArray<FString>& AppendFileExtersions,
	const TArray<FString>& IgnoreFormats, const TArray<FString>& InIgnoreOptions)
{
	ParallelFor(OriginCommands.Num(),[&](int32 index)
	{
		FString& Command = OriginCommands[index];
		FString PakOptionsStr;
		for (const auto& Param : Options)
		{
			FString FileExtension = UFlibHotPatcherCoreHelper::GetPakCommandExtersion(Command);
			if(IgnoreFormats.Contains(FileExtension) && InIgnoreOptions.Contains(Param))
			{
				continue;
			}
							
			FString AppendOptionStr = TEXT("");
			if(bAppendAllMatch || AppendFileExtersions.Contains(FileExtension))
			{
				AppendOptionStr += TEXT(" ") + Param;
			}
							
			PakOptionsStr += AppendOptionStr;
		}
		Command = FString::Printf(TEXT("%s%s"),*Command,*PakOptionsStr);
	});
}

FProjectPackageAssetCollection UFlibHotPatcherCoreHelper::ImportProjectSettingsPackages()
{
	FProjectPackageAssetCollection result;
	TArray<FDirectoryPath>& DirectoryPaths = result.DirectoryPaths;
	TArray<FSoftObjectPath>& SoftObjectPaths = result.NeedCookPackages;
	
	auto AddSoftObjectPath = [&](const FString& LongPackageName)
	{
		bool bSuccessed = false;
		// PRAGMA_DISABLE_DEPRECATION_WARNINGS
		if (UFlibHotPatcherCoreHelper::IsCanCookPackage(LongPackageName))
		{
			FString LongPackagePath = LongPackageName.Contains(TEXT(".")) ? LongPackageName : UFlibAssetManageHelper::LongPackageNameToPackagePath(LongPackageName);
			FSoftObjectPath CurrentObject(LongPackagePath);
			if(FPackageName::DoesPackageExist(LongPackagePath) && CurrentObject.IsValid()) 
			{
				// UE_LOG(LogHotPatcherCoreHelper,Display,TEXT("Import Project Setting Package: %s"),*LongPackagePath);
				SoftObjectPaths.AddUnique(CurrentObject);
				bSuccessed = true;
			}
		}
		// PRAGMA_ENABLE_DEPRECATION_WARNINGS
		if(!bSuccessed)
		{
			// UE_LOG(LogHotPatcherCoreHelper,Warning,TEXT("Import Project Setting Package: %s is inavlid!"),*LongPackagePath);
		}
	};
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry* AssetRegistry = &AssetRegistryModule.Get();
	
	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
	
	{
		// allow the game to fill out the asset registry, as well as get a list of objects to always cook
		TArray<FString> FilesInPathStrings;
		PRAGMA_DISABLE_DEPRECATION_WARNINGS;
		FGameDelegates::Get().GetCookModificationDelegate().ExecuteIfBound(FilesInPathStrings);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS;
		for(const auto& BuildFilename:FilesInPathStrings)
		{
			FString OutPackageName;
			if (FPackageName::TryConvertFilenameToLongPackageName(FPaths::ConvertRelativePathToFull(BuildFilename), OutPackageName))
			{
				AddSoftObjectPath(OutPackageName);
			}
		}
	}
	
	// in Asset Manager / PrimaryAssetLabel
	{
		TArray<FName> PackageToCook;
		TArray<FName> PackageToNeverCook;
#if ENGINE_MAJOR_VERSION > 4
		ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
		const TArray<ITargetPlatform*>& Platforms = TPM->GetActiveTargetPlatforms();
		UAssetManager::Get().ModifyCook(Platforms, PackageToCook, PackageToNeverCook);
#else
		UAssetManager::Get().ModifyCook(PackageToCook, PackageToNeverCook);
#endif


		for(const auto& Package:PackageToCook)
		{
			AddSoftObjectPath(Package.ToString());
		}
		for(const auto& NeverCookPackage:PackageToNeverCook)
		{
			result.NeverCookPackages.Add(NeverCookPackage.ToString());
		}
	}

	// DirectoriesToAlwaysCook
	DirectoryPaths.Append(PackagingSettings->DirectoriesToAlwaysCook);

	// AlwaysCookMaps
	{
		TArray<FString> MapList;
		// Add the default map section
		GEditor->LoadMapListFromIni(TEXT("AlwaysCookMaps"), MapList);

		for (int32 MapIdx = 0; MapIdx < MapList.Num(); MapIdx++)
		{
			FName PackageName = FName(*FPackageName::FilenameToLongPackageName(FPaths::ConvertRelativePathToFull(MapList[MapIdx])));
			AddSoftObjectPath(PackageName.ToString());
		}
	}

	// MapsToCook
	for (const FFilePath& MapToCook : PackagingSettings->MapsToCook)
	{
		FString File = MapToCook.FilePath;
		FName PackageName = FName(*FPackageName::FilenameToLongPackageName(FPaths::ConvertRelativePathToFull(File)));
		AddSoftObjectPath(PackageName.ToString());
	}

	// Loading default map ini section AllMaps
	{
		TArray<FString> AllMapsSection;
		GEditor->LoadMapListFromIni(TEXT("AllMaps"), AllMapsSection);
		for (const FString& MapName : AllMapsSection)
		{
			AddSoftObjectPath(MapName);
		}
	}

	// all uasset and umap
	if(!SoftObjectPaths.Num() && !DirectoryPaths.Num())
	{
		TArray<FString> Tokens;
		Tokens.Empty(2);
		Tokens.Add(FString("*") + FPackageName::GetAssetPackageExtension());
		Tokens.Add(FString("*") + FPackageName::GetMapPackageExtension());

		uint8 PackageFilter = NORMALIZE_DefaultFlags | NORMALIZE_ExcludeEnginePackages | NORMALIZE_ExcludeLocalizedPackages;
		bool bMapsOnly = false;
		if (bMapsOnly)
		{
			PackageFilter |= NORMALIZE_ExcludeContentPackages;
		}
		bool bNoDev = false;
		if (bNoDev)
		{
			PackageFilter |= NORMALIZE_ExcludeDeveloperPackages;
		}

		// assume the first token is the map wildcard/pathname
		TArray<FString> Unused;
		for (int32 TokenIndex = 0; TokenIndex < Tokens.Num(); TokenIndex++)
		{
			TArray<FString> TokenFiles;
			if (!NormalizePackageNames(Unused, TokenFiles, Tokens[TokenIndex], PackageFilter))
			{
				UE_LOG(LogHotPatcherCoreHelper, Display, TEXT("No packages found for parameter %i: '%s'"), TokenIndex, *Tokens[TokenIndex]);
				continue;
			}

			for (int32 TokenFileIndex = 0; TokenFileIndex < TokenFiles.Num(); ++TokenFileIndex)
			{
				FName PackageName = FName(*FPackageName::FilenameToLongPackageName(FPaths::ConvertRelativePathToFull(TokenFiles[TokenFileIndex])));
				AddSoftObjectPath(PackageName.ToString());
			}
		}
	}
	
	// ===============================================

	{
		TSet<FName> StartupPackages;
		TSet<FName> StartupSoftObjectPackages;

		for (TObjectIterator<UPackage> It; It; ++It)
		{
			if ((*It) != GetTransientPackage())
			{
				if(!It->GetFName().ToString().StartsWith(TEXT("/Script")))
				{
					StartupPackages.Add(It->GetFName());
				}
			}
		}
		
		// Get the list of soft references, for both empty package and all startup packages
		GRedirectCollector.ProcessSoftObjectPathPackageList(NAME_None, false, StartupSoftObjectPackages);

		for (const FName& StartupPackage : StartupPackages)
		{
			GRedirectCollector.ProcessSoftObjectPathPackageList(StartupPackage, false, StartupSoftObjectPackages);
		}

		// Add string asset packages after collecting files, to avoid accidentally activating the behavior to cook all maps if none are specified
		for (FName SoftObjectPackage : StartupSoftObjectPackages)
		{
			AddSoftObjectPath(SoftObjectPackage.ToString());
		}
	}
	// Find all the localized packages and map them back to their source package
	{
		TArray<FString> AllCulturesToCook;
		for (const FString& CultureName : PackagingSettings->CulturesToStage)
		{
			const TArray<FString> PrioritizedCultureNames = FInternationalization::Get().GetPrioritizedCultureNames(CultureName);
			for (const FString& PrioritizedCultureName : PrioritizedCultureNames)
			{
				AllCulturesToCook.AddUnique(PrioritizedCultureName);
			}
		}
		AllCulturesToCook.Sort();

		TArray<FString> RootPaths;
		FPackageName::QueryRootContentPaths(RootPaths);

		FARFilter Filter;
		Filter.bRecursivePaths = true;
		Filter.bIncludeOnlyOnDiskAssets = false;
		Filter.PackagePaths.Reserve(AllCulturesToCook.Num() * RootPaths.Num());
		for (const FString& RootPath : RootPaths)
		{
			for (const FString& CultureName : AllCulturesToCook)
			{
				FString LocalizedPackagePath = RootPath / TEXT("L10N") / CultureName;
				Filter.PackagePaths.Add(*LocalizedPackagePath);
			}
		}

		TArray<FAssetData> AssetDataForCultures;
		AssetRegistry->GetAssets(Filter, AssetDataForCultures);

		for (const FAssetData& AssetData : AssetDataForCultures)
		{
			const FName LocalizedPackageName = AssetData.PackageName;
			const FName SourcePackageName = *FPackageName::GetSourcePackagePath(LocalizedPackageName.ToString());
			AddSoftObjectPath(LocalizedPackageName.ToString());
		}
	}
	const UGameMapsSettings* const GameMapsSettings = GetDefault<UGameMapsSettings>();
	{
		if(GameMapsSettings->GameInstanceClass.IsAsset())
		{
			AddSoftObjectPath(GameMapsSettings->GameInstanceClass.GetAssetPathString());
		}
	
		FSoftObjectPath GameDefaultMap{
			(GameMapsSettings->GetGameDefaultMap())
		};
		if(GameDefaultMap.IsAsset())
		{
			AddSoftObjectPath(GameDefaultMap.GetAssetPathString());
		}
	
		FSoftObjectPath DefaultGameMode{
			(GameMapsSettings->GetGlobalDefaultGameMode())
		};
		if(DefaultGameMode.IsAsset())
		{
			AddSoftObjectPath(DefaultGameMode.GetAssetPathString());
		}
	
		if(GameMapsSettings->TransitionMap.IsAsset())
		{
			AddSoftObjectPath(GameMapsSettings->TransitionMap.GetAssetPathString());
		}
	}
	auto CreateDirectory = [](const FString& Path)
	{
		FDirectoryPath DirectoryPath;
		DirectoryPath.Path = Path;
		return DirectoryPath;
	};

	

	// DirectoryPaths.AddUnique(CreateDirectory("/Game/UI"));
	// DirectoryPaths.AddUnique(CreateDirectory("/Game/Widget"));
	// DirectoryPaths.AddUnique(CreateDirectory("/Game/Widgets"));
	// DirectoryPaths.AddUnique(CreateDirectory("/Engine/MobileResources"));
	{
		TArray<FString> UIContentPaths;
		TSet <FName> ContentDirectoryAssets; 
		if (GConfig->GetArray(TEXT("UI"), TEXT("ContentDirectories"), UIContentPaths, GEditorIni) > 0)
		{
			for (int32 DirIdx = 0; DirIdx < UIContentPaths.Num(); DirIdx++)
			{
				DirectoryPaths.Add(CreateDirectory(UIContentPaths[DirIdx]));
			}
		}
	}

	{
		FConfigFile InputIni;
		FString InterfaceFile;
		FConfigCacheIni::LoadLocalIniFile(InputIni, TEXT("Input"), true);
		if (InputIni.GetString(TEXT("/Script/Engine.InputSettings"), TEXT("DefaultTouchInterface"), InterfaceFile))
		{
			if (InterfaceFile != TEXT("None") && InterfaceFile != TEXT(""))
			{
				AddSoftObjectPath(InterfaceFile);
			}
		}
	}

	{
		TArray<FString> AllCulturesToCook = PackagingSettings->CulturesToStage;;
		
		TArray<FString> RootPaths;
		FPackageName::QueryRootContentPaths(RootPaths);

		FARFilter Filter;
		Filter.bRecursivePaths = true;
		Filter.bIncludeOnlyOnDiskAssets = false;
		Filter.PackagePaths.Reserve(AllCulturesToCook.Num() * RootPaths.Num());
		for (const FString& RootPath : RootPaths)
		{
			for (const FString& CultureName : AllCulturesToCook)
			{
				FString LocalizedPackagePath = RootPath / TEXT("L10N") / CultureName;
				Filter.PackagePaths.Add(*LocalizedPackagePath);
			}
		}

		
		TArray<FAssetData> AssetDataForCultures;
		AssetRegistry->GetAssets(Filter, AssetDataForCultures);

		for (const FAssetData& AssetData : AssetDataForCultures)
		{
			// const FName LocalizedPackageName = AssetData.PackageName;
			// const FName SourcePackageName = *FPackageName::GetSourcePackagePath(LocalizedPackageName.ToString());

			AddSoftObjectPath(AssetData.PackageName.ToString());
		}
	}

	// GetUnsolicitedPackages
	{
		for (TObjectIterator<UPackage> It; It; ++It)
		{
			UPackage* Package = *It;

			if (Package->GetOuter() == nullptr)
			{
				AddSoftObjectPath(Package->GetName());
			}
		}
	}
	
	// never cook packages
	result.NeverCookPaths.Append(PackagingSettings->DirectoriesToNeverCook);

	return result;
}

void UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites()
{
	SCOPED_NAMED_EVENT_TEXT("WaitForAsyncFileWrites",FColor::Red);
	// UE_LOG(LogHotPatcher, Display, TEXT("Wait For Async File Writes..."));
	TSharedPtr<FThreadWorker> WaitThreadWorker = MakeShareable(new FThreadWorker(TEXT("WaitCookComplete"),[]()
		{
			UPackage::WaitForAsyncFileWrites();
		}));
	WaitThreadWorker->Execute();
	WaitThreadWorker->Join();
}

void UFlibHotPatcherCoreHelper::WaitDDCComplete()
{
	SCOPED_NAMED_EVENT_TEXT("WaitDDCComplete",FColor::Red);
	GetDerivedDataCacheRef().WaitForQuiescence(true);
}

bool UFlibHotPatcherCoreHelper::IsCanCookPackage(const FString& LongPackageName)
{
	bool bResult = false;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (!LongPackageName.IsEmpty() && !FPackageName::IsScriptPackage(LongPackageName) && !FPackageName::IsMemoryPackage(LongPackageName))
	{
		bResult = UAssetManager::Get().VerifyCanCookPackage(FName(*LongPackageName),false);
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	return bResult;
}

void UFlibHotPatcherCoreHelper::ImportProjectSettingsToScannerConfig(FAssetScanConfig& AssetScanConfig)
{
	FProjectPackageAssetCollection AssetCollection = UFlibHotPatcherCoreHelper::ImportProjectSettingsPackages();
	AssetScanConfig.AssetIncludeFilters.Append(AssetCollection.DirectoryPaths);

	for(const auto& Asset:AssetCollection.NeedCookPackages)
	{
		FPatcherSpecifyAsset CurrentAsset;
		CurrentAsset.Asset = Asset;
		CurrentAsset.bAnalysisAssetDependencies = true;
		CurrentAsset.AssetRegistryDependencyTypes = {EAssetRegistryDependencyTypeEx::Packages};
		AssetScanConfig.IncludeSpecifyAssets.Add(CurrentAsset);
	}
	// NEVER COOK
	AssetScanConfig.AssetIgnoreFilters.Append(AssetCollection.NeverCookPaths);
	AssetScanConfig.ForceSkipContentRules.Append(AssetCollection.NeverCookPaths);
	AssetScanConfig.ForceSkipAssets.Append(AssetCollection.NeverCookPackages);
}

void UFlibHotPatcherCoreHelper::ImportProjectNotAssetDir(TArray<FPlatformExternAssets>& PlatformExternAssets, ETargetPlatform AddToTargetPlatform)
{
	const auto ProjectNotAssetDirs = UFlibHotPatcherCoreHelper::GetProjectNotAssetDirConfig();
	FPlatformExternAssets* AddToPlatformExternalAssetsPtr = nullptr;

	for(auto& AddExternAssetsToPlatform:PlatformExternAssets)
	{
		if(AddExternAssetsToPlatform.TargetPlatform == AddToTargetPlatform)
		{
			AddToPlatformExternalAssetsPtr = &AddExternAssetsToPlatform;
		}
	}
	if(!AddToPlatformExternalAssetsPtr)
	{
		FPlatformExternAssets AllPlatformExternalAssets;
		AllPlatformExternalAssets.TargetPlatform = AddToTargetPlatform;
				
		int32 index = PlatformExternAssets.Add(AllPlatformExternalAssets);
		AddToPlatformExternalAssetsPtr = &PlatformExternAssets[index];
	}
	AddToPlatformExternalAssetsPtr->AddExternDirectoryToPak.Append(ProjectNotAssetDirs);
}

TArray<FExternDirectoryInfo> UFlibHotPatcherCoreHelper::GetProjectNotAssetDirConfig()
{
	TArray<FExternDirectoryInfo> result;
	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();

	FString BasePath = FString::Printf(TEXT("../../../%s/Content/"),FApp::GetProjectName());
	auto FixPath = [](const FString& BasePath,const FString& Path)->FString
	{
		FString result;
		FString finalSubPath = Path;
		TArray<FString> PathsItem = UKismetStringLibrary::ParseIntoArray(BasePath,TEXT("/"));
		while(PathsItem.Num() && finalSubPath.StartsWith(TEXT("../")))
		{
			PathsItem.RemoveAt(PathsItem.Num()-1);
			finalSubPath.RemoveFromStart(TEXT("../"));
		}
		PathsItem.Add(finalSubPath);
		for(auto& DirItem:PathsItem)
		{
			if(!DirItem.IsEmpty())
			{
				result = FPaths::Combine(result,DirItem);
			}
		}
		return result;
	};
	
	if(PackagingSettings)
	{
		for(auto ContentSubDir:PackagingSettings->DirectoriesToAlwaysStageAsUFS)
		{
			FExternDirectoryInfo DirInfo;
			
			FString MountPoint = ContentSubDir.Path;
			if(MountPoint.StartsWith(TEXT("../")))
			{
				MountPoint = FixPath(BasePath,MountPoint);
				DirInfo.DirectoryPath.Path = FixPath(TEXT("[PROJECTDIR]/Content"),ContentSubDir.Path);
			}
			else
			{
				MountPoint = FPaths::Combine(BasePath,MountPoint);
				DirInfo.DirectoryPath.Path = FString::Printf(TEXT("[PROJECT_CONTENT_DIR]/%s"),*ContentSubDir.Path);
			}
			DirInfo.MountPoint = MountPoint;
			FPaths::NormalizeDirectoryName(DirInfo.MountPoint);
			result.Emplace(DirInfo);
		}
	}
	return result;
}

void UFlibHotPatcherCoreHelper::CacheForCookedPlatformData(
	const TArray<FSoftObjectPath>& ObjectPaths,
	TArray<ITargetPlatform*> TargetPlatforms,
	TSet<UObject*>& ProcessedObjs,
	TSet<UObject*>& PendingCachePlatformDataObjects,
	bool bStorageConcurrent,
	bool bWaitComplete,
	TFunction<void(UPackage*,UObject*)> OnPreCacheObjectWithOuter
	)
{
	SCOPED_NAMED_EVENT_TEXT("CacheForCookedPlatformData",FColor::Red);
	TArray<UPackage*> AllPackages = UFlibAssetManageHelper::LoadPackagesForCooking(ObjectPaths,bStorageConcurrent);
	{
		SCOPED_NAMED_EVENT_TEXT("BeginCacheForCookedPlatformData for Assets",FColor::Red);
		UFlibHotPatcherCoreHelper::CacheForCookedPlatformData(AllPackages,TargetPlatforms,ProcessedObjs,PendingCachePlatformDataObjects, bStorageConcurrent, bWaitComplete);
	}

	if(bWaitComplete)
	{
		SCOPED_NAMED_EVENT_TEXT("WaitCacheForCookedPlatformDataComplete",FColor::Red);
		// Wait for all shaders to finish compiling
		UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
		// UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
	}
}


#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
UE_TRACE_EVENT_BEGIN(CUSTOM_LOADTIMER_LOG, CachePackagePlatformData, NoSync)
	UE_TRACE_EVENT_FIELD(Trace::WideString, PackageName)
UE_TRACE_EVENT_END()

#endif
void UFlibHotPatcherCoreHelper::CacheForCookedPlatformData(
	const TArray<UPackage*>& Packages,
	TArray<ITargetPlatform*> TargetPlatforms,
	TSet<UObject*>& ProcessedObjs,
	TSet<UObject*>& PendingCachePlatformDataObjects,
	bool bStorageConcurrent,
	bool bWaitComplete,
	TFunction<void(UPackage*,UObject*)> OnPreCacheObjectWithOuter
	)
{
	SCOPED_NAMED_EVENT_TEXT("CacheForCookedPlatformData",FColor::Red);
	
	TMap<UWorld*, bool> WorldsToPostSaveRoot;
	WorldsToPostSaveRoot.Reserve(1024);
	
	for(auto Package:Packages)
	{
		FString LongPackageName = UFlibAssetManageHelper::LongPackageNameToPackagePath(Package->GetPathName());
		// FExecTimeRecoder PreGeneratePlatformDataTimer(FString::Printf(TEXT("PreGeneratePlatformData %s"),*LongPackageName));
		FString FakePackageName = FString(TEXT("Package ")) + LongPackageName;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
		SCOPED_CUSTOM_LOADTIMER(CachePackagePlatformData)
			ADD_CUSTOM_LOADTIMER_META(CachePackagePlatformData, PackageName, *FakePackageName);
#else
		FScopedNamedEvent CachePackagePlatformDataEvent(FColor::Red,*FString::Printf(TEXT("%s"),*LongPackageName));
#endif
		
		if(!Package)
    	{
    		UE_LOG(LogHotPatcher,Warning,TEXT("BeginPackageObjectsCacheForCookedPlatformData Package is null!"));
    		continue;
    	}
    	
    	{
    		SCOPED_NAMED_EVENT_TEXT("ExportMap BeginCacheForCookedPlatformData",FColor::Red);
			
    		uint32 SaveFlags = UFlibHotPatcherCoreHelper::GetCookSaveFlag(Package,true,bStorageConcurrent,false);
    		TArray<UObject*> ObjectsInPackage;
    		GetObjectsWithOuter(Package,ObjectsInPackage,true);
    		for(const auto& ExportObj:ObjectsInPackage)
    		{
    			if(OnPreCacheObjectWithOuter)
    			{
    				OnPreCacheObjectWithOuter(Package,ExportObj);
    			}
#if ENGINE_MINOR_VERSION < 26
    			FScopedNamedEvent CacheExportEvent(FColor::Red,*FString::Printf(TEXT("%s"),*ExportObj->GetName()));
#endif
    			if (ExportObj->HasAnyFlags(RF_Transient))
    			{
    				// UE_LOG(LogHotPatcherCoreHelper, Display, TEXT("%s is PreCached."),*ExportObj->GetFullName());
    				continue;
    			}
    			if(ProcessedObjs.Contains(ExportObj))
    			{
    				continue;;
    			}
    			
    			bool bInitializedPhysicsSceneForSave = false;
    			bool bForceInitializedWorld = false;
    			UWorld* World = Cast<UWorld>(ExportObj);
    			if (World && bStorageConcurrent)
    			{
    				// We need a physics scene at save time in case code does traces during onsave events.
    				bInitializedPhysicsSceneForSave = GEditor->InitializePhysicsSceneForSaveIfNecessary(World, bForceInitializedWorld);
    	
    				GIsCookerLoadingPackage = true;
    				{
    					GEditor->OnPreSaveWorld(SaveFlags, World);
    				}
    				{
    					bool bCleanupIsRequired = World->PreSaveRoot(TEXT(""));
    					WorldsToPostSaveRoot.Add(World, bCleanupIsRequired);
    				}
    				GIsCookerLoadingPackage = false;
    			}
    			
    			if(ExportObj->GetClass()->GetName().Equals(TEXT("LandscapeComponent")) && bStorageConcurrent)
    			{
    				UE_LOG(LogHotPatcherCoreHelper,Display,TEXT("Object %s is a LandscapeComponent"),*ExportObj->GetFullName());
    				TArray<UTexture2D*>* MobileWeightmapTextures = nullptr;
    				for(TFieldIterator<FProperty> PropertyIter(ExportObj->GetClass());PropertyIter;++PropertyIter)
    				{
    					FProperty* PropertyIns = *PropertyIter;
    					if(PropertyIns->GetName().Equals(TEXT("MobileWeightmapTextures")))
    					{
    						MobileWeightmapTextures = PropertyIns->ContainerPtrToValuePtr<TArray<UTexture2D*>>(ExportObj);
    						break;
    					}
    				}
    				if(MobileWeightmapTextures)
    				{
    					UE_LOG(LogHotPatcherCoreHelper,Display,TEXT("Add %s MobileWeightmapTextures to ObjectsInPackage"),*ExportObj->GetFullName());
    					for(UObject* MobileWeightmapTexture:*MobileWeightmapTextures)
    					{
    						ObjectsInPackage.AddUnique(MobileWeightmapTexture);
    					}
    				}
    			}
    			
    			for(const auto& Platform:TargetPlatforms)
    			{
    				if (bStorageConcurrent)
    				{
    					SCOPED_NAMED_EVENT_TEXT("Export PreSave",FColor::Red);
    					GIsCookerLoadingPackage = true;
    					{
    						ExportObj->PreSave(Platform);
    					}
    					GIsCookerLoadingPackage = false;
    				}
    				
    				// bool bIsTexture = ExportObj->IsA(UTexture::StaticClass());
    				// if (!bIsTexture || bStorageConcurrent)
    				{
    					SCOPED_NAMED_EVENT_TEXT("BeginCacheForCookedPlatformData",FColor::Red);
    					
    					UPackage* Outermost = ExportObj->GetOutermost();
    					bool bHasFilterEditorOnly = Outermost->HasAnyPackageFlags(PKG_FilterEditorOnly);
    					bool bIsTransient = (Outermost == GetTransientPackage());
    					bool bIsTexture = ExportObj->IsA(UTexture::StaticClass());
    					
    					ExportObj->BeginCacheForCookedPlatformData(Platform);
    					if(!(bIsTexture && bHasFilterEditorOnly) && !ExportObj->IsCachedCookedPlatformDataLoaded(Platform))
    					{
    						PendingCachePlatformDataObjects.Add(ExportObj);
    					}
    					if(ExportObj->IsCachedCookedPlatformDataLoaded(Platform))
    					{
    						ProcessedObjs.Add(ExportObj);
    					}
    				}
    			}
    	
    			if (World && bInitializedPhysicsSceneForSave)
    			{
    				SCOPED_NAMED_EVENT_TEXT("CleanupPhysicsSceneThatWasInitializedForSave",FColor::Red);
    				GEditor->CleanupPhysicsSceneThatWasInitializedForSave(World, bForceInitializedWorld);
    			}
    		}
    	}
		if (bStorageConcurrent)
		{
			// Precache the metadata so we don't risk rehashing the map in the parallelfor below
			Package->GetMetaData();
		}
	}

		
	if (bStorageConcurrent)
	{
		// UE_LOG(LogHotPatcherCoreHelper, Display, TEXT("Calling PostSaveRoot on worlds..."));
		for (auto WorldIt = WorldsToPostSaveRoot.CreateConstIterator(); WorldIt; ++WorldIt)
		{
#if ENGINE_MINOR_VERSION < 26
			FScopedNamedEvent CacheExportEvent(FColor::Red,*FString::Printf(TEXT("World PostSaveRoot")));
#endif
			UWorld* World = WorldIt.Key();
			check(World);
			World->PostSaveRoot(WorldIt.Value());
		}
	}
	
	// When saving concurrently, flush async loading since that is normally done internally in SavePackage
	if (bStorageConcurrent)
	{
#if ENGINE_MINOR_VERSION < 26
		FScopedNamedEvent CacheExportEvent(FColor::Red,*FString::Printf(TEXT("FlushAsyncLoading and ProcessThreadUtilIdle")));
#endif
		FlushAsyncLoading();
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	}
	
	if(bWaitComplete)
	{
		UFlibHotPatcherCoreHelper::WaitObjectsCachePlatformDataComplete(ProcessedObjs,PendingCachePlatformDataObjects,TargetPlatforms);
	}

}

void UFlibHotPatcherCoreHelper::WaitObjectsCachePlatformDataComplete(TSet<UObject*>& CachedPlatformDataObjects,TSet<UObject*>& PendingCachePlatformDataObjects,
	TArray<ITargetPlatform*> TargetPlatforms)
{
	SCOPED_NAMED_EVENT_TEXT("WaitObjectsCachePlatformDataComplete",FColor::Red);
	// FExecTimeRecoder WaitObjectsCachePlatformDataCompleteTimer(TEXT("WaitObjectsCachePlatformDataComplete"));
	if (GShaderCompilingManager)
	{
		// Wait for all shaders to finish compiling
		UFlibShaderCodeLibraryHelper::WaitShaderCompilingComplete();
	}
	UFlibHotPatcherCoreHelper::WaitDistanceFieldAsyncQueueComplete();
	
	{
		SCOPED_NAMED_EVENT_TEXT("FlushAsyncLoading And WaitingAsyncTasks",FColor::Red);
		FlushAsyncLoading();
		UE_LOG(LogHotPatcherCoreHelper, Display, TEXT("Waiting for async tasks..."));
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	}
	
	{
		SCOPED_NAMED_EVENT_TEXT("WaitCachePlatformDataComplete",FColor::Red);
		while(!!PendingCachePlatformDataObjects.Num())
		{
			TSet<UObject*> CachedObjects;
			for(auto Object:PendingCachePlatformDataObjects)
			{
				bool bAllPlatformDataLoaded = true;
				for (const ITargetPlatform* TargetPlatform : TargetPlatforms)
				{
					// check cache status
					if (!Object->IsCachedCookedPlatformDataLoaded(TargetPlatform))
					{
						bAllPlatformDataLoaded = false;
						break;
					}else{
						if(GCookLog)
						{
							UE_LOG(LogHotPatcherCoreHelper,Log,TEXT("PreCached ExportObj %s for %s"),*Object->GetFullName(),*TargetPlatform->PlatformName());
						}
					}
				}
				// add to cached set
				if (bAllPlatformDataLoaded)
				{
					CachedPlatformDataObjects.Add(Object);
					CachedObjects.Add(Object);
				}
			}

			// remove cached
			for(auto CachedObject:CachedObjects)
			{
				PendingCachePlatformDataObjects.Remove(CachedObject);
			}
			
			if(!!PendingCachePlatformDataObjects.Num())
			{
			#if ENGINE_MAJOR_VERSION > 4
				// call ProcessAsyncTasks instead of pure wait using Sleep
				while (FAssetCompilingManager::Get().GetNumRemainingAssets() > 0)
				{
					// Process any asynchronous Asset compile results that are ready, limit execution time
					FAssetCompilingManager::Get().ProcessAsyncTasks(true);
				}
			#else
				FPlatformProcess::Sleep(0.1f);
			#endif // ENGINE_MAJOR_VERSION > 4
				GLog->Flush();
			}
		}
	}
	// UFlibHotPatcherCoreHelper::WaitForAsyncFileWrites();
}

uint32 UFlibHotPatcherCoreHelper::GetCookSaveFlag(UPackage* Package, bool bUnversioned, bool bStorageConcurrent,
                                                  bool CookLinkerDiff)
{
	uint32 SaveFlags = SAVE_KeepGUID | SAVE_Async| SAVE_ComputeHash | (bUnversioned ? SAVE_Unversioned : 0);

#if ENGINE_MAJOR_VERSION >4 || ENGINE_MINOR_VERSION >25
	// bool CookLinkerDiff = false;
	if(CookLinkerDiff)
	{
		SaveFlags |= SAVE_CompareLinker;
	}
#endif
	if (bStorageConcurrent)
	{
		SaveFlags |= SAVE_Concurrent;
	}
	return SaveFlags;
}

EObjectFlags UFlibHotPatcherCoreHelper::GetObjectFlagForCooked(UPackage* Package)
{
	EObjectFlags CookedFlags = RF_Public;
	if(UWorld::FindWorldInPackage(Package))
	{
		CookedFlags = RF_NoFlags;
	}
	return CookedFlags;
}


void UFlibHotPatcherCoreHelper::SaveGlobalShaderMapFiles(const TArrayView<const ITargetPlatform* const>& Platforms,const FString& BaseOutputDir)
{
	// we don't support this behavior
	for (int32 Index = 0; Index < Platforms.Num(); Index++)
	{
		// make sure global shaders are up to date!
		TArray<FString> Files;
		FShaderRecompileData RecompileData;
		RecompileData.PlatformName = Platforms[Index]->PlatformName();
		// Compile for all platforms
		RecompileData.ShaderPlatform = SP_NumPlatforms;
		RecompileData.ModifiedFiles = &Files;
		RecompileData.MeshMaterialMaps = NULL;

		check( IsInGameThread() );

		FString OutputDir  = FPaths::Combine(BaseOutputDir,Platforms[Index]->PlatformName());

#if ENGINE_MAJOR_VERSION > 4
		TArray<uint8> GlobalShaderMap;
		RecompileData.CommandType = ODSCRecompileCommand::Global;
		RecompileData.GlobalShaderMap = &GlobalShaderMap;
		RecompileShadersForRemote(RecompileData, OutputDir);
#else
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
#endif
	}
}

FString UFlibHotPatcherCoreHelper::GetSavePackageResultStr(ESavePackageResult Result)
{
	FString Str;

	switch (Result)
	{
	case ESavePackageResult::Success:
		{
			Str = TEXT("Success");
			break;
		}
	case ESavePackageResult::Canceled:
		{
			Str = TEXT("Canceled");
			break;
		}
	case ESavePackageResult::Error:
		{
			Str = TEXT("Error");
			break;
		}
	case ESavePackageResult::DifferentContent:
		{
			Str = TEXT("DifferentContent");
			break;
		}
	case ESavePackageResult::GenerateStub:
		{
			Str = TEXT("GenerateStub");
			break;
		}
	case ESavePackageResult::MissingFile:
		{
			Str = TEXT("MissingFile");
			break;
		}
	case ESavePackageResult::ReplaceCompletely:
		{
			Str = TEXT("ReplaceCompletely");
			break;
		}
	case ESavePackageResult::ContainsEditorOnlyData:
		{
			Str = TEXT("ContainsEditorOnlyData");
			break;
		}
	case ESavePackageResult::ReferencedOnlyByEditorOnlyData:
		{
			Str = TEXT("ReferencedOnlyByEditorOnlyData");
			break;
		}
	}
	return Str;
}

void UFlibHotPatcherCoreHelper::AdaptorOldVersionConfig(FAssetScanConfig& ScanConfig, const FString& JsonContent)
{
	THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(JsonContent,ScanConfig);
}


bool UFlibHotPatcherCoreHelper::GetIniPlatformName(const FString& InPlatformName, FString& OutIniPlatformName)
{
	bool bStatus = false;
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	for (ITargetPlatform *TargetPlatformIns : TargetPlatforms)
	{
		FString PlatformName = TargetPlatformIns->PlatformName();
		if(PlatformName.Equals(InPlatformName))
		{
			OutIniPlatformName = TargetPlatformIns->IniPlatformName();
			bStatus = true;
			break;
		}
	}
	return bStatus;
}

#if GENERATE_CHUNKS_MANIFEST
#include "Commandlets/AssetRegistryGenerator.h"
#include "Commandlets/AssetRegistryGenerator.cpp"
#endif

bool UFlibHotPatcherCoreHelper::SerializeChunksManifests(ITargetPlatform* TargetPlatform, const TSet<FName>& CookedPackageNames, const TSet<FName>& IgnorePackageNames, bool
															 bGenerateStreamingInstallManifest)
{
	bool bresult = true;
#if GENERATE_CHUNKS_MANIFEST
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 26
	TUniquePtr<class FSandboxPlatformFile> TempSandboxFile = MakeUnique<FSandboxPlatformFile>(false);
#else
	TUniquePtr<class FSandboxPlatformFile> TempSandboxFile = FSandboxPlatformFile::Create(false);
#endif
	FString PlatformSandboxDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked"),TargetPlatform->PlatformName()));
	// Use SandboxFile to do path conversion to properly handle sandbox paths (outside of standard paths in particular).
	TempSandboxFile->Initialize(&FPlatformFileManager::Get().GetPlatformFile(), *FString::Printf(TEXT("-sandbox=\"%s\""), *PlatformSandboxDir));
	TUniquePtr<FAssetRegistryGenerator> RegistryGenerator = MakeUnique<FAssetRegistryGenerator>(TargetPlatform);
	RegistryGenerator->CleanManifestDirectories();
	RegistryGenerator->Initialize(TArray<FName>());
	RegistryGenerator->PreSave(CookedPackageNames);
#if ENGINE_MAJOR_VERSION > 4	
	RegistryGenerator->FinalizeChunkIDs(CookedPackageNames, IgnorePackageNames, *TempSandboxFile, bGenerateStreamingInstallManifest);
#else
	RegistryGenerator->BuildChunkManifest(CookedPackageNames, IgnorePackageNames, TempSandboxFile.Get(), bGenerateStreamingInstallManifest);
#endif
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 26)
	FString TempSandboxManifestDir = FPaths::Combine(PlatformSandboxDir,FApp::GetProjectName(),TEXT("Metadata") , TEXT("ChunkManifest"));
	if(!FPaths::DirectoryExists(TempSandboxManifestDir))
	{
		IFileManager::Get().MakeDirectory(*TempSandboxManifestDir, true);
	}
#endif
	bresult = RegistryGenerator->SaveManifests(
#if ENGINE_MAJOR_VERSION > 4
	*TempSandboxFile
#else
TempSandboxFile.Get()
#endif
	);
#endif
	return bresult;
}

TArray<UClass*> UFlibHotPatcherCoreHelper::GetClassesByNames(const TArray<FName>& ClassesNames)
{
	SCOPED_NAMED_EVENT_TEXT("GetClassesByNames",FColor::Red);
	TArray<UClass*> result;
	for(const auto& ClassesName:ClassesNames)
	{
		for (TObjectIterator<UClass> Itt; Itt; ++Itt)
		{
			if((*Itt)->GetName().Equals(ClassesName.ToString()))
			{
				result.Add(*Itt);
				break;
			}
		}
	}
	return result;
}

TArray<UClass*> UFlibHotPatcherCoreHelper::GetAllMaterialClasses()
{
	SCOPED_NAMED_EVENT_TEXT("GetAllMaterialClasses",FColor::Red);
	TArray<UClass*> Classes;
	TArray<FName> ParentClassesName = {
		// materials
		TEXT("MaterialExpression"),
		TEXT("MaterialParameterCollection"),
		TEXT("MaterialFunctionInterface"),
		TEXT("Material"),
		TEXT("MaterialInterface"),
		};
	for(auto& ParentClass:GetClassesByNames(ParentClassesName))
	{
		Classes.Append(UFlibHotPatcherCoreHelper::GetDerivedClasses(ParentClass,true,true));
	}
	return Classes;
}

bool UFlibHotPatcherCoreHelper::IsMaterialClasses(UClass* Class)
{
	return UFlibHotPatcherCoreHelper::IsMaterialClassName(Class->GetFName());
};

bool UFlibHotPatcherCoreHelper::IsMaterialClassName(FName ClassName)
{
	return UFlibHotPatcherCoreHelper::GetAllMaterialClassesNames().Contains(ClassName);
}

bool UFlibHotPatcherCoreHelper::AssetDetailsHasClasses(const TArray<FAssetDetail>& AssetDetails, TSet<FName> ClasssName)
{
	SCOPED_NAMED_EVENT_TEXT("AssetDetailsHasClasses",FColor::Red);
	bool bHas = false;
	for(const auto& Detail:AssetDetails)
	{
		if(ClasssName.Contains(Detail.AssetType))
		{
			bHas = true;
			break;
		}
	}
	return bHas;
}

TSet<FName> UFlibHotPatcherCoreHelper::GetAllMaterialClassesNames()
{
	SCOPED_NAMED_EVENT_TEXT("GetAllMaterialClassesNames",FColor::Red);
	TSet<FName> result;
	for(const auto& Class:GetAllMaterialClasses())
	{
		result.Add(Class->GetFName());
	}
	return result;
}

TArray<UClass*> UFlibHotPatcherCoreHelper::GetPreCacheClasses()
{
	SCOPED_NAMED_EVENT_TEXT("GetPreCacheClasses",FColor::Red);
	TArray<UClass*> Classes;
	
	TArray<FName> ParentClassesName = {
		// textures
		TEXT("Texture"),
		TEXT("PaperSprite"),
		// material
		// TEXT("MaterialExpression"),
		// TEXT("MaterialParameterCollection"),
		// TEXT("MaterialFunctionInterface"),
		// TEXT("MaterialInterface"),
		// other
		TEXT("PhysicsAsset"),
		TEXT("PhysicalMaterial"),
		TEXT("StaticMesh"),
		// curve
		TEXT("CurveFloat"),
		TEXT("CurveVector"),
		TEXT("CurveLinearColor"),
		// skeletal and animation
		TEXT("Skeleton"),
		TEXT("SkeletalMesh"),
		TEXT("AnimSequence"),
		TEXT("BlendSpace1D"),
		TEXT("BlendSpace"),
		TEXT("AnimMontage"),
		TEXT("AnimComposite"),
		// blueprint
		TEXT("UserDefinedStruct"),
		TEXT("Blueprint"),
		// sound
		TEXT("SoundWave"),
		// particles
		TEXT("FXSystemAsset"),
		// large ref asset
		TEXT("ActorSequence"),
		TEXT("LevelSequence"),
		TEXT("World") 
	};

	for(auto& ParentClass:UFlibHotPatcherCoreHelper::GetClassesByNames(ParentClassesName))
	{
		Classes.Append(UFlibHotPatcherCoreHelper::GetDerivedClasses(ParentClass,true,true));
	}
	Classes.Append(UFlibHotPatcherCoreHelper::GetAllMaterialClasses());
	TSet<UClass*> Results;
	for(const auto& Class:Classes)
	{
		Results.Add(Class);
	}
	return Results.Array();
}

void UFlibHotPatcherCoreHelper::DumpActiveTargetPlatforms()
{
	FString ActiveTargetPlatforms;
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM && (TPM->RestrictFormatsToRuntimeOnly() == false))
	{
		TArray<ITargetPlatform*> Platforms = TPM->GetActiveTargetPlatforms();
		for(auto& Platform:Platforms)
		{
			ActiveTargetPlatforms += FString::Printf(TEXT("%s,"),*Platform->PlatformName());
		}
		ActiveTargetPlatforms.RemoveFromEnd(TEXT(","));
	}
	UE_LOG(LogHotPatcherCoreHelper,Display,TEXT("[IMPORTTENT] ActiveTargetPlatforms: %s"),*ActiveTargetPlatforms);
}

FString UFlibHotPatcherCoreHelper::GetPlatformsStr(TArray<ETargetPlatform> Platforms)
{
	return UFlibPatchParserHelper::GetPlatformsStr(Platforms);
}

#include "DistanceFieldAtlas.h"
void UFlibHotPatcherCoreHelper::WaitDistanceFieldAsyncQueueComplete()
{
	SCOPED_NAMED_EVENT_TEXT("WaitDistanceFieldAsyncQueueComplete",FColor::Red);
	if (GDistanceFieldAsyncQueue)
	{
		UE_LOG(LogHotPatcherCoreHelper, Display, TEXT("Waiting for distance field async operations..."));
		GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();
	}
}