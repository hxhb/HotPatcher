// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibHotPatcherEditorHelper.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "HotPatcherLog.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "HAL/PlatformFilemanager.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Editor.h"
#include "IPlatformFileSandboxWrapper.h"
#include "Async/Async.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/SecureHash.h"

DEFINE_LOG_CATEGORY(LogHotPatcherEditorHelper);

TArray<FString> UFlibHotPatcherEditorHelper::GetAllCookOption()
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

void UFlibHotPatcherEditorHelper::CreateSaveFileNotify(const FText& InMsg, const FString& InSavedFile,SNotificationItem::ECompletionState NotifyType)
{
	AsyncTask(ENamedThreads::GameThread,[InMsg,InSavedFile,NotifyType]()
	{
		auto Message = InMsg;
		FNotificationInfo Info(Message);
		Info.bFireAndForget = true;
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = false;
		Info.bUseLargeFont = false;

		const FString HyperLinkText = InSavedFile;
		Info.Hyperlink = FSimpleDelegate::CreateLambda(
			[](FString SourceFilePath)
			{
				FPlatformProcess::ExploreFolder(*SourceFilePath);
			},
			HyperLinkText
		);
		Info.HyperlinkText = FText::FromString(HyperLinkText);
		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(NotifyType);
	});
}

void UFlibHotPatcherEditorHelper::CheckInvalidCookFilesByAssetDependenciesInfo(
	const FString& InProjectAbsDir, 
	const FString& InPlatformName, 
	const FAssetDependenciesInfo& InAssetDependencies, 
	TArray<FAssetDetail>& OutValidAssets, 
	TArray<FAssetDetail>& OutInvalidAssets)
{
	OutValidAssets.Empty();
	OutInvalidAssets.Empty();
	TArray<FAssetDetail> AllAssetDetails;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(InAssetDependencies,AllAssetDetails);

	for (const auto& AssetDetail : AllAssetDetails)
	{
		TArray<FString> CookedAssetPath;
		TArray<FString> CookedAssetRelativePath;
		FString AssetLongPackageName;
		if(!UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(AssetDetail.mPackagePath, AssetLongPackageName))
			continue;
		FAssetData CurrentAssetData;
		UFLibAssetManageHelperEx::GetSingleAssetsData(AssetDetail.mPackagePath,CurrentAssetData);
		if (CurrentAssetData.GetAsset()->HasAnyMarks(OBJECTMARK_EditorOnly))
		{
			UE_LOG(LogHotPatcherEditorHelper,Warning,TEXT("Miss %s it's EditorOnly Assets!"),*CurrentAssetData.PackageName.ToString());
			continue;
		}
		
		if (UFLibAssetManageHelperEx::ConvLongPackageNameToCookedPath(
			InProjectAbsDir,
			InPlatformName,
			AssetLongPackageName,
			CookedAssetPath,
			CookedAssetRelativePath))
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

FChunkInfo UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(const FExportPatchSettings* InPatchSetting)
{
	FChunkInfo Chunk;
	if (!(InPatchSetting && InPatchSetting))
	{
		return Chunk;
	}
	
	Chunk.ChunkName = InPatchSetting->VersionId;
	Chunk.bMonolithic = false;
	Chunk.MonolithicPathMode = EMonolithicPathMode::MountPath;
	Chunk.bStorageUnrealPakList = InPatchSetting->GetUnrealPakSettings().bStoragePakList;
	Chunk.bStorageIoStorePakList = InPatchSetting->GetIoStoreSettings().bStoragePakList;
	Chunk.AssetIncludeFilters = const_cast<FExportPatchSettings*>(InPatchSetting)->GetAssetIncludeFilters();
	Chunk.AssetIgnoreFilters = const_cast<FExportPatchSettings*>(InPatchSetting)->GetAssetIgnoreFilters();
	Chunk.bAnalysisFilterDependencies = InPatchSetting->IsAnalysisFilterDependencies();
	Chunk.IncludeSpecifyAssets = const_cast<FExportPatchSettings*>(InPatchSetting)->GetIncludeSpecifyAssets();
	Chunk.AddExternAssetsToPlatform = const_cast<FExportPatchSettings*>(InPatchSetting)->GetAddExternAssetsToPlatform();
	Chunk.AssetRegistryDependencyTypes = InPatchSetting->GetAssetRegistryDependencyTypes();
	Chunk.InternalFiles.bIncludeAssetRegistry = InPatchSetting->IsIncludeAssetRegistry();
	Chunk.InternalFiles.bIncludeGlobalShaderCache = InPatchSetting->IsIncludeGlobalShaderCache();
	Chunk.InternalFiles.bIncludeShaderBytecode = InPatchSetting->IsIncludeShaderBytecode();
	Chunk.InternalFiles.bIncludeEngineIni = InPatchSetting->IsIncludeEngineIni();
	Chunk.InternalFiles.bIncludePluginIni = InPatchSetting->IsIncludePluginIni();
	Chunk.InternalFiles.bIncludeProjectIni = InPatchSetting->IsIncludeProjectIni();

	return Chunk;
}

FChunkInfo UFlibHotPatcherEditorHelper::MakeChunkFromPatchVerison(const FHotPatcherVersion& InPatchVersion)
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
	TArray<FAssetDetail> AllVersionAssets;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(InPatchVersion.AssetInfo, AllVersionAssets);

	for (const auto& Asset : AllVersionAssets)
	{
		FPatcherSpecifyAsset CurrentAsset;
		CurrentAsset.Asset = FSoftObjectPath(Asset.mPackagePath);
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
	FString ProjectContentAbsir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	if(FileName.StartsWith(ProjectContentAbsir))
	{
		FString GameFileName = FileName;
		GameFileName.RemoveFromStart(ProjectContentAbsir);
		Result = FPaths::Combine(FApp::GetProjectName(),TEXT("Content"),GameFileName);
		return Result;
	}
	if(FileName.StartsWith(FPaths::EngineContentDir()))
	{
		FString EngineFileName = FileName;
		EngineFileName.RemoveFromStart(FPaths::EngineContentDir());
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
			// UE_LOG(LogHotPatcherEditorHelper,Log,TEXT("Plugin Content:%s"),*PluginContentDir);
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

FString UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(const FString& BaseDir, const FString PacakgeName, const FString& Platform)
{
	FString Filename;
	FString PackageFilename;
	FString StandardFilename;
	FName StandardFileFName = NAME_None;

	if (FPackageName::DoesPackageExist(PacakgeName, NULL, &Filename, false))
	{
		StandardFilename = PackageFilename = FPaths::ConvertRelativePathToFull(Filename);

		FPaths::MakeStandardFilename(StandardFilename);
		StandardFileFName = FName(*StandardFilename);
	}

	FString SandboxFilename = ConvertToFullSandboxPath(*StandardFilename, true);
	// UE_LOG(LogHotPatcherEditorHelper,Log,TEXT("Filename:%s,PackageFileName:%s,StandardFileName:%s"),*Filename,*PackageFilename,*StandardFilename);
	
	FString CookDir =FPaths::Combine(BaseDir,Platform,SandboxFilename);
	
	return 	CookDir;
}

FString UFlibHotPatcherEditorHelper::GetProjectCookedDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));;
}

bool UFlibHotPatcherEditorHelper::CookAssets(
	const TArray<FSoftObjectPath>& Assets,
	const TArray<ETargetPlatform>&Platforms,
	TFunction<void(const FString&)> PackageSavedCallback,
    class TMap<ETargetPlatform,FSavePackageContext*> PlatformSavePackageContext
)
{
	TArray<FAssetData> AssetsData;
	TArray<UPackage*> Packages;
	for(const auto& Asset:Assets)
	{
		FAssetData AssetData;
		if(UFLibAssetManageHelperEx::GetSingleAssetsData(Asset.GetAssetPathString(),AssetData))
		{
			AssetsData.AddUnique(AssetData);
			Packages.AddUnique(AssetData.GetPackage());
		}
	}
	TArray<FString> StringPlatforms;
	TMap<FString,FSavePackageContext*> FinalPlatformSavePackageContext;
	for(const auto& Platform:Platforms)
	{
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
		StringPlatforms.AddUnique(PlatformName);
		FSavePackageContext* CurrentPackageContext = NULL;
		if(PlatformSavePackageContext.Contains(Platform))
		{
			CurrentPackageContext = *PlatformSavePackageContext.Find(Platform);
		}
		 
		FinalPlatformSavePackageContext.Add(PlatformName,CurrentPackageContext);
	}
	
	return CookPackages(AssetsData,Packages,StringPlatforms,PackageSavedCallback,FinalPlatformSavePackageContext);
}

bool UFlibHotPatcherEditorHelper::CookPackages(
	const TArray<FAssetData>& AssetDatas,
	const TArray<UPackage*>& InPackage,
	const TArray<FString>& Platforms,
	TFunction<void(const FString&)> PackageSavedCallback,
	class TMap<FString,FSavePackageContext*> PlatformSavePackageContext
)
{
	if(AssetDatas.Num()!=InPackage.Num())
		return false;
	for(int32 index=0;index<InPackage.Num();++index)
	{
		CookPackage(AssetDatas[index],InPackage[index],Platforms,PackageSavedCallback,PlatformSavePackageContext);
	}
	return true;
}


bool UFlibHotPatcherEditorHelper::CookPackage(
	const FAssetData& AssetData,
	UPackage* Package,
	const TArray<FString>& Platforms,
	//const FString& SavePath,
	TFunction<void(const FString&)> PackageSavedCallback,
	class TMap<FString,FSavePackageContext*> PlatformSavePackageContext
)
{
	bool bSuccessed = false;
	const bool bStorageConcurrent = FParse::Param(FCommandLine::Get(), TEXT("ConcurrentSave"));
	bool bUnversioned = true;
	uint32 SaveFlags = SAVE_KeepGUID | SAVE_Async| SAVE_ComputeHash | (bUnversioned ? SAVE_Unversioned : 0);

#if ENGINE_MAJOR_VERSION >4 || ENGINE_MINOR_VERSION >25
	bool CookLinkerDiff = false;
	if(CookLinkerDiff)
	{
		SaveFlags |= SAVE_CompareLinker;
	}
#endif
	
	EObjectFlags CookedFlags = RF_Public;
	if(Cast<UWorld>(Package))
	{
		CookedFlags = RF_NoFlags;
	}
	if (bStorageConcurrent)
	{
		SaveFlags |= SAVE_Concurrent;
	}
	TArray<ITargetPlatform*> CookPlatforms; 
	for (auto const& PlatformName : Platforms)
	{
		ITargetPlatform* Platform = GetPlatformByName(PlatformName);
		if(Platform)
		{
			CookPlatforms.Add(Platform);
		}
	}
	FString SavePath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),TEXT("Cooked"));
#if ENGINE_MAJOR_VERSION > 4
	FName PackageFileName = Package->GetLoadedPath().GetPackageFName();
#else
	FName PackageFileName = Package->FileName;
#endif
	if(PackageFileName.IsNone() && AssetData.PackageName.IsNone())
		return bSuccessed;
	for(auto& Platform:CookPlatforms)
	{
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

		FFilterEditorOnlyFlag SetPackageEditorOnlyFlag(Package,Platform);

		FString PackageName = PackageFileName.IsNone()?AssetData.PackageName.ToString():PackageFileName.ToString();
		FString CookedSavePath = UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(SavePath,PackageName, Platform->PlatformName());
		// delete old cooked assets
		if(FPaths::FileExists(CookedSavePath))
		{
			IFileManager::Get().Delete(*CookedSavePath);
		}
		// UE_LOG(LogHotPatcherEditorHelper,Log,TEXT("Cook Assets:%s"),*Package->GetName());
		Package->FullyLoad();
		TArray<UObject*> ExportMap;
		GetObjectsWithOuter(Package,ExportMap);
		for(const auto& ExportObj:ExportMap)
		{
			ExportObj->BeginCacheForCookedPlatformData(Platform);
		}

		// if(!bStorageConcurrent)
		// {
		// 	TArray<UObject*> TagExpObjects;
		// 	GetObjectsWithAnyMarks(TagExpObjects,OBJECTMARK_TagExp);
		// 	for(const auto& TagExportObj:TagExpObjects)
		// 	{
		// 		if(TagExportObj->HasAnyMarks(OBJECTMARK_TagExp))
		// 		{
		// 			TagExportObj->BeginCacheForCookedPlatformData(Platform);
		// 		}
		// 	}
		// }
#if WITH_PACKAGE_CONTEXT
		FSavePackageContext* CurrentPlatformPackageContext = nullptr;
		if(PlatformSavePackageContext.Contains(Platform->PlatformName()))
			CurrentPlatformPackageContext = *PlatformSavePackageContext.Find(Platform->PlatformName());
#endif
		GIsCookerLoadingPackage = true;
		UPackage::PackageSavedEvent.AddLambda([PackageSavedCallback](const FString& InFilePath,UObject* Object){PackageSavedCallback(InFilePath);});
		
		// UE_LOG(LogHotPatcherEditorHelper,Display,TEXT("Cook Assets:%s save to %s"),*Package->GetName(),*CookedSavePath);
		FSavePackageResultStruct Result = GEditor->Save(	Package, nullptr, CookedFlags, *CookedSavePath, 
                                                GError, nullptr, false, false, SaveFlags, Platform, 
                                                FDateTime::MinValue(), false, /*DiffMap*/ nullptr
#if WITH_PACKAGE_CONTEXT
                                                ,CurrentPlatformPackageContext
#endif
                                                );
		GIsCookerLoadingPackage = false;
		bSuccessed = Result == ESavePackageResult::Success;
	}
	return bSuccessed;
}


void UFlibHotPatcherEditorHelper::CookChunkAssets(
	TArray<FAssetDetail> Assets,
	const TArray<ETargetPlatform>& Platforms,
	class TMap<ETargetPlatform,FSavePackageContext*> PlatformSavePackageContext
)
{
	TArray<FSoftObjectPath> AssetsSoftPath;

	for(const auto& Asset:Assets)
	{
		FSoftObjectPath AssetSoftPath;
		AssetSoftPath.SetPath(Asset.mPackagePath);
		AssetsSoftPath.AddUnique(AssetSoftPath);
	}
	if(!!AssetsSoftPath.Num())
	{
		UFlibHotPatcherEditorHelper::CookAssets(AssetsSoftPath,Platforms);
	}
}

ITargetPlatform* UFlibHotPatcherEditorHelper::GetTargetPlatformByName(const FString& PlatformName)
{
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	ITargetPlatform* PlatformIns = NULL; 
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		if (PlatformName.Contains(TargetPlatform->PlatformName()))
		{
			PlatformIns = TargetPlatform;
		}
	}
	return PlatformIns;
}

TArray<ITargetPlatform*> UFlibHotPatcherEditorHelper::GetTargetPlatformsByNames(const TArray<ETargetPlatform>& Platforms)
{

	TArray<ITargetPlatform*> result;
	for(const auto& Platform:Platforms)
	{
    			
		ITargetPlatform* Found = UFlibHotPatcherEditorHelper::GetTargetPlatformByName(UFlibPatchParserHelper::GetEnumNameByValue(Platform,false));
		if(Found)
		{
			result.Add(Found);
		}
	}
	return result;
}



FString UFlibHotPatcherEditorHelper::GetUnrealPakBinary()
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

FString UFlibHotPatcherEditorHelper::GetUECmdBinary()
{
	FString Binary;
#if ENGINE_MAJOR_VERSION > 4
	Binary = TEXT("UnrealEditor");
#else
	Binary = TEXT("UE4Editor");
#endif

	
#if PLATFORM_WINDOWS
	return FPaths::Combine(
        FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
        TEXT("Binaries"),
#if PLATFORM_64BITS	
        TEXT("Win64"),
#else
        TEXT("Win32"),
#endif
#ifdef WITH_HOTPATCHER_DEBUGGAME
	#if PLATFORM_64BITS
			FString::Printf(TEXT("%s-Win64-DebugGame-Cmd.exe"),*Binary)
	        // TEXT("UE4Editor-Win64-DebugGame-Cmd.exe")
	#else
			FString::Printf(TEXT("%s-Win32-DebugGame-Cmd.exe"),*Binary)
	        // TEXT("UE4Editor-Win32-DebugGame-Cmd.exe")
	#endif
#else
		FString::Printf(TEXT("%s-Cmd.exe"),*Binary)
        // TEXT("UE4Editor-Cmd.exe")
#endif
    );
#endif
#if PLATFORM_MAC
	return FPaths::Combine(
        FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
        TEXT("Binaries"),
        TEXT("Mac"),
        FString::Printf(TEXT("%s-Cmd"),*Binary)
        //TEXT("UE4Editor-Cmd")
    );
#endif
	return TEXT("");
}


FProcHandle UFlibHotPatcherEditorHelper::DoUnrealPak(TArray<FString> UnrealPakCommandletOptions, bool block)
{
	FString UnrealPakBinary = UFlibHotPatcherEditorHelper::GetUnrealPakBinary();

	FString CommandLine;
	for (const auto& Option : UnrealPakCommandletOptions)
	{
		CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
	}

	// create UnrealPak process

	uint32 *ProcessID = NULL;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*UnrealPakBinary, *CommandLine, true, false, false, ProcessID, 0, NULL, NULL, NULL);

	if (ProcHandle.IsValid())
	{
		if (block)
		{
			FPlatformProcess::WaitForProc(ProcHandle);
		}
	}
	return ProcHandle;
}

FString UFlibHotPatcherEditorHelper::GetMetadataDir(const FString& ProjectDir, const FString& ProjectName,ETargetPlatform Platform)
{
	FString result;
	FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform,false);
	return FPaths::Combine(ProjectDir,TEXT("Saved/Cooked"),PlatformName,ProjectName,TEXT("Metadata"));
}

void UFlibHotPatcherEditorHelper::BackupMetadataDir(const FString& ProjectDir, const FString& ProjectName,
	const TArray<ETargetPlatform>& Platforms, const FString& OutDir)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	for(const auto& Platform:Platforms)
	{
		FString MetadataDir = FPaths::ConvertRelativePathToFull(UFlibHotPatcherEditorHelper::GetMetadataDir(ProjectDir,ProjectName,Platform));
		FString OutMetadir = FPaths::Combine(OutDir,TEXT("BackupMatedatas"),UFlibPatchParserHelper::GetEnumNameByValue(Platform,false));
		if(FPaths::DirectoryExists(MetadataDir))
		{
			PlatformFile.CreateDirectoryTree(*OutMetadir);
			PlatformFile.CopyDirectoryTree(*OutMetadir,*MetadataDir,true);
		}
	}
}

FString UFlibHotPatcherEditorHelper::ReleaseSummary(const FHotPatcherVersion& NewVersion)
{
	FString result = TEXT("\n---------------------HotPatcher Release Summary---------------------\n");

	auto ParserPlatformExternAssets = [](ETargetPlatform Platform,const FPlatformExternAssets& PlatformExternAssets)->FString
	{
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);;
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
	result += FString::Printf(TEXT("New Release Asset Number: %d\n"),UFLibAssetManageHelperEx::ParserAssetDependenciesInfoNumber(NewVersion.AssetInfo,AddModuleAssetNumMap));
	result += UFLibAssetManageHelperEx::ParserModuleAssetsNumMap(AddModuleAssetNumMap);
	TArray<ETargetPlatform> Keys;
	NewVersion.PlatformAssets.GetKeys(Keys);
	for(const auto& Key:Keys)
	{
		result += ParserPlatformExternAssets(Key,*NewVersion.PlatformAssets.Find(Key));
	}
	return result;
}

FString UFlibHotPatcherEditorHelper::PatchSummary(const FPatchVersionDiff& DiffInfo)
{
	auto ExternFileSummary = [](ETargetPlatform Platform,const FPatchVersionExternDiff& ExternDiff)->FString
	{
		FString PlatformName = UFlibPatchParserHelper::GetEnumNameByValue(Platform);;
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
	result += FString::Printf(TEXT("Add Asset Number: %d\n"),UFLibAssetManageHelperEx::ParserAssetDependenciesInfoNumber(DiffInfo.AssetDiffInfo.AddAssetDependInfo,AddModuleAssetNumMap));
	result += UFLibAssetManageHelperEx::ParserModuleAssetsNumMap(AddModuleAssetNumMap);
	TMap<FString,uint32> ModifyModuleAssetNumMap;
	result += FString::Printf(TEXT("Modify Asset Number: %d\n"),UFLibAssetManageHelperEx::ParserAssetDependenciesInfoNumber(DiffInfo.AssetDiffInfo.ModifyAssetDependInfo,ModifyModuleAssetNumMap));
	result += UFLibAssetManageHelperEx::ParserModuleAssetsNumMap(ModifyModuleAssetNumMap);
	TMap<FString,uint32> DeleteModuleAssetNumMap;
	result += FString::Printf(TEXT("Delete Asset Number: %d\n"),UFLibAssetManageHelperEx::ParserAssetDependenciesInfoNumber(DiffInfo.AssetDiffInfo.DeleteAssetDependInfo,DeleteModuleAssetNumMap));
	result += UFLibAssetManageHelperEx::ParserModuleAssetsNumMap(DeleteModuleAssetNumMap);
	TArray<ETargetPlatform> Platforms;
	DiffInfo.PlatformExternDiffInfo.GetKeys(Platforms);
	for(const auto& Platform:Platforms)
	{
		result += ExternFileSummary(Platform,*DiffInfo.PlatformExternDiffInfo.Find(Platform));
	}
	return result;
}


FString UFlibHotPatcherEditorHelper::MakePakShortName(const FHotPatcherVersion& InCurrentVersion, const FChunkInfo& InChunkInfo, const FString& InPlatform,const FString& InRegular)
{
	struct FResularOperator
	{
		FResularOperator(const FString& InName,TFunction<FString(void)> InOperator)
			:Name(InName),Do(InOperator){}
		FString Name;
		TFunction<FString(void)> Do;
	};
	
	TArray<FResularOperator> RegularOpList;
	RegularOpList.Emplace(TEXT("{VERSION}"),[&InCurrentVersion]()->FString{return InCurrentVersion.VersionId;});
	RegularOpList.Emplace(TEXT("{BASEVERSION}"),[&InCurrentVersion]()->FString{return InCurrentVersion.BaseVersionId;});
	RegularOpList.Emplace(TEXT("{PLATFORM}"),[&InPlatform]()->FString{return InPlatform;});
	RegularOpList.Emplace(TEXT("{CHUNKNAME}"),[&InChunkInfo,&InCurrentVersion]()->FString
	{
		if(!InCurrentVersion.VersionId.Equals(InChunkInfo.ChunkName))
			return InChunkInfo.ChunkName;
		else
			return TEXT("");
	});
	
	auto CustomPakNameRegular = [](const TArray<FResularOperator>& Operators,const FString& Regular)->FString
	{
		FString Result = Regular;
		for(auto& Operator:Operators)
		{
			Result = Result.Replace(*Operator.Name,*(Operator.Do()));
		}
		while(Result.Contains(TEXT("__")))
		{
			Result = Result.Replace(TEXT("__"),TEXT("_"));
		}
		return Result;
	};
	
	return CustomPakNameRegular(RegularOpList,InRegular);
}

bool UFlibHotPatcherEditorHelper::CheckSelectedAssetsCookStatus(const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg)
{
	OutMsg.Empty();
	// 检查所修改的资源是否被Cook过
	for (const auto& PlatformName : PlatformNames)
	{
		TArray<FAssetDetail> ValidCookAssets;
		TArray<FAssetDetail> InvalidCookAssets;

		UFlibHotPatcherEditorHelper::CheckInvalidCookFilesByAssetDependenciesInfo(UKismetSystemLibrary::GetProjectDirectory(), PlatformName, SelectedAssets, ValidCookAssets, InvalidCookAssets);

		if (InvalidCookAssets.Num() > 0)
		{
			OutMsg.Append(FString::Printf(TEXT("%s UnCooked Assets:\n"), *PlatformName));

			for (const auto& Asset : InvalidCookAssets)
			{
				FString AssetLongPackageName;
				UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(Asset.mPackagePath, AssetLongPackageName);
				OutMsg.Append(FString::Printf(TEXT("\t%s\n"), *AssetLongPackageName));
			}
		}
	}

	return OutMsg.IsEmpty();
}

bool UFlibHotPatcherEditorHelper::CheckPatchRequire(const FPatchVersionDiff& InDiff,const TArray<FString>& PlatformNames,FString& OutMsg)
{
	bool Status = false;
	// 错误处理
	{
		FString GenErrorMsg;
		FAssetDependenciesInfo AllChangedAssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(InDiff.AssetDiffInfo.AddAssetDependInfo, InDiff.AssetDiffInfo.ModifyAssetDependInfo);
		bool bSelectedCookStatus = CheckSelectedAssetsCookStatus(PlatformNames, AllChangedAssetInfo, GenErrorMsg);

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

FString UFlibHotPatcherEditorHelper::Conv2IniPlatform(const FString& Platform)
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

TArray<FString> UFlibHotPatcherEditorHelper::GetSupportPlatforms()
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


FString UFlibHotPatcherEditorHelper::GetEncryptSettingsCommandlineOptions(const FPakEncryptSettings& EncryptSettings,const FString& PlatformName)
{
	FString Result; 
	if(EncryptSettings.bEncrypt)
	{
		Result += EncryptSettings.bEncrypt? TEXT("-encrypt "):TEXT("");
		Result += EncryptSettings.bEncryptIndex? TEXT("-encryptindex "):TEXT("");
		
		FString CryptoKey = UFlibPatchParserHelper::ReplaceMarkPath(EncryptSettings.CryptoKeys.FilePath);
		if(FPaths::FileExists(CryptoKey))
		{
			Result += FString::Printf(TEXT("-crypto=\"%s\" "),*CryptoKey);
		}
	}
	if(EncryptSettings.bUseDefaultCryptoIni)
		Result += EncryptSettings.bUseDefaultCryptoIni? TEXT("-encryptionini "):TEXT("");
	if(EncryptSettings.bSign)
		Result += EncryptSettings.bSign? TEXT("-sign "):TEXT("");
	
	Result += FString::Printf(TEXT("-projectdir=\"%s\" "),*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
	Result += FString::Printf(TEXT("-enginedir=\"%s\" "),*FPaths::ConvertRelativePathToFull(FPaths::EngineDir()));
	Result += FString::Printf(TEXT("-platform=%s"),*PlatformName);
	return Result;
}

ITargetPlatform* UFlibHotPatcherEditorHelper::GetPlatformByName(const FString& Name)
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