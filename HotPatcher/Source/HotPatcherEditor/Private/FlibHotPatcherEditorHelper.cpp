// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibHotPatcherEditorHelper.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "HotPatcherLog.h"

// engine header
#include "IPlatformFileSandboxWrapper.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/SecureHash.h"

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
		UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(AssetDetail.mPackagePath, AssetLongPackageName);
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
	Chunk.bSavePakCommands = true;
	Chunk.AssetIncludeFilters = InPatchSetting->GetAssetIncludeFilters();
	Chunk.AssetIgnoreFilters = InPatchSetting->GetAssetIgnoreFilters();
	Chunk.bAnalysisFilterDependencies = InPatchSetting->IsAnalysisFilterDependencies();
	Chunk.IncludeSpecifyAssets = InPatchSetting->GetIncludeSpecifyAssets();
	Chunk.AddExternAssetsToPlatform = InPatchSetting->GetAddExternAssetsToPlatform();
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
	Chunk.bSavePakCommands = false;
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
	FString ProjectContentAbsir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	if(FileName.StartsWith(ProjectContentAbsir))
	{
		FString GameFileName = FileName;
		GameFileName.RemoveFromStart(ProjectContentAbsir);
		return FPaths::Combine(FApp::GetProjectName(),TEXT("Content"),GameFileName);
	}
	if(FileName.StartsWith(FPaths::EngineContentDir()))
	{
		FString EngineFileName = FileName;
		EngineFileName.RemoveFromStart(FPaths::EngineContentDir());
		return FPaths::Combine(TEXT("Engine/Content"),EngineFileName);;
	}
	TArray<TSharedRef<IPlugin> > PluginsToRemap = IPluginManager::Get().GetEnabledPlugins();
	// Ideally this would be in the Sandbox File but it can't access the project or plugin
	if (PluginsToRemap.Num() > 0)
	{
		// Handle remapping of plugins
		for (TSharedRef<IPlugin> Plugin : PluginsToRemap)
		{
			FString PluginContentDir = Plugin->GetContentDir();
			if (FileName.StartsWith(PluginContentDir))
			{
				FString SearchFor;
				SearchFor /= Plugin->GetName() / TEXT("Content");
				int32 FoundAt = FileName.Find(SearchFor, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				check(FoundAt != -1);
				// Strip off everything but <PluginName/Content/<remaing path to file>
				FString SnippedOffPath = FileName.RightChop(FoundAt);

				FString LoadingFrom;
				switch(Plugin->GetLoadedFrom())
				{
				case EPluginLoadedFrom::Engine:
					{
						LoadingFrom = TEXT("Engine/Plugins");
						break;
					}
				case EPluginLoadedFrom::Project:
					{
						LoadingFrom = FPaths::Combine(FApp::GetProjectName(),TEXT("Plugins"));
						break;
					}
				}
					
				return FPaths::Combine(LoadingFrom,SnippedOffPath);
			}
		}
	}

	return TEXT("");
}

FString UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(const FString& BaseDir, UPackage* Package, const FString& Platform)
{
	FString Filename;
	FString PackageFilename;
	FString StandardFilename;
	FName StandardFileFName = NAME_None;

	if (FPackageName::DoesPackageExist(Package->FileName.ToString(), NULL, &Filename, false))
	{
		StandardFilename = PackageFilename = FPaths::ConvertRelativePathToFull(Filename);

		FPaths::MakeStandardFilename(StandardFilename);
		StandardFileFName = FName(*StandardFilename);
	}

	FString SandboxFilename = ConvertToFullSandboxPath(*StandardFilename, true);
	
	FString CookDir =FPaths::Combine(BaseDir,Platform,SandboxFilename);
	return 	CookDir;
}

FString UFlibHotPatcherEditorHelper::GetProjectCookedDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));;
}



bool UFlibHotPatcherEditorHelper::CookAssets(const TArray<FSoftObjectPath>& Assets, const TArray<ETargetPlatform>&Platforms,
                                            const FString& SavePath)
{
	FString FinalSavePath = SavePath;
	if(FinalSavePath.IsEmpty())
	{
		FinalSavePath = UFlibHotPatcherEditorHelper::GetProjectCookedDir();
	}
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
	for(const auto& Platform:Platforms)
	{
		StringPlatforms.AddUnique(UFlibPatchParserHelper::GetEnumNameByValue(Platform));
	}
	
	return CookPackages(Packages,StringPlatforms,SavePath);
}

bool UFlibHotPatcherEditorHelper::CookPackage(UPackage* Package, const TArray<FString>& Platforms, const FString& SavePath)
{
	bool bSuccessed = false;
	const bool bSaveConcurrent = FParse::Param(FCommandLine::Get(), TEXT("ConcurrentSave"));
	bool bUnversioned = false;
	uint32 SaveFlags = SAVE_KeepGUID | SAVE_Async | SAVE_ComputeHash | (bUnversioned ? SAVE_Unversioned : 0);
	EObjectFlags CookedFlags = RF_Public;
	if(Cast<UWorld>(Package))
	{
		CookedFlags = RF_NoFlags;
	}
	if (bSaveConcurrent)
	{
		SaveFlags |= SAVE_Concurrent;
	}
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	TArray<ITargetPlatform*> CookPlatforms; 
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		if (Platforms.Contains(TargetPlatform->PlatformName()))
		{
			CookPlatforms.AddUnique(TargetPlatform);
		}
	}
	if(Package->FileName.IsNone())
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
		
		FString CookedSavePath = UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(SavePath,Package, Platform->PlatformName());
		// delete old cooked assets
		if(FPaths::FileExists(CookedSavePath))
		{
			IFileManager::Get().Delete(*CookedSavePath);
		}
		
		GIsCookerLoadingPackage = true;
		FSavePackageResultStruct Result = GEditor->Save(	Package, nullptr, CookedFlags, *CookedSavePath, 
                                                GError, nullptr, false, false, SaveFlags, Platform, 
                                                FDateTime::MinValue(), false, /*DiffMap*/ nullptr);
		GIsCookerLoadingPackage = false;
		bSuccessed = Result == ESavePackageResult::Success;
	}
	return bSuccessed;
}

bool UFlibHotPatcherEditorHelper::CookPackages(TArray<UPackage*>& InPackage, const TArray<FString>& Platforms, const FString& SavePath)
{
	for(const auto& Package:InPackage)
	{
		CookPackage(Package,Platforms,SavePath);
	}
	return true;
}

