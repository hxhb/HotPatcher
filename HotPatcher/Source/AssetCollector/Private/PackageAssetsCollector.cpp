// Fill out your copyright notice in the Description page of Project Settings.


#include "PackageAssetsCollector.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Engine/AssetManager.h"
#include "Interfaces/ITargetPlatform.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Interfaces/IPluginManager.h"

IAssetRegistry* GetAssetRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry* AssetRegistry = &AssetRegistryModule.Get();
	return AssetRegistry;
}


void UPackageAssetsCollector::DiscoverPlatformSpecificNeverCookPackages(
	const TArrayView<ITargetPlatform*>& TargetPlatforms, const TArray<FString>& UBTPlatformStrings)
{
	TArray<const ITargetPlatform*> PluginUnsupportedTargetPlatforms;
	TArray<FAssetData> PluginAssets;
	FARFilter PluginARFilter;
	FString PluginPackagePath;

	IAssetRegistry* AssetRegistry = GetAssetRegistry();
	
	TArray<TSharedRef<IPlugin>> AllContentPlugins = IPluginManager::Get().GetEnabledPluginsWithContent();
	for (TSharedRef<IPlugin> Plugin : AllContentPlugins)
	{
		const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();

		// we are only interested in plugins that does not support all platforms
		if (Descriptor.SupportedTargetPlatforms.Num() == 0)
		{
			continue;
		}

		// find any unsupported target platforms for this plugin
		PluginUnsupportedTargetPlatforms.Reset();
		for (int32 I = 0, Count = TargetPlatforms.Num(); I < Count; ++I)
		{
			if (!Descriptor.SupportedTargetPlatforms.Contains(UBTPlatformStrings[I]))
			{
				PluginUnsupportedTargetPlatforms.Add(TargetPlatforms[I]);
			}
		}

		// if there are unsupported target platforms,
		// then add all packages for this plugin for these platforms to the PlatformSpecificNeverCookPackages map
		if (PluginUnsupportedTargetPlatforms.Num() > 0)
		{
			PluginPackagePath.Reset(127);
			PluginPackagePath.AppendChar(TEXT('/'));
			PluginPackagePath.Append(Plugin->GetName());

			PluginARFilter.bRecursivePaths = true;
			PluginARFilter.bIncludeOnlyOnDiskAssets = true;
			PluginARFilter.PackagePaths.Reset(1);
			PluginARFilter.PackagePaths.Emplace(*PluginPackagePath);

			PluginAssets.Reset();
			AssetRegistry->GetAssets(PluginARFilter, PluginAssets);

			for (const ITargetPlatform* TargetPlatform: PluginUnsupportedTargetPlatforms)
			{
				TSet<FName>& NeverCookPackages = CookTracker.PlatformSpecificNeverCookPackages.FindOrAdd(TargetPlatform);
				for (const FAssetData& Asset : PluginAssets)
				{
					NeverCookPackages.Add(Asset.PackageName);
				}
			}
		}
	}
}

bool UPackageAssetsCollector::IsCookFlagSet( const ECookInitializationFlags& InCookFlags ) 
{
	return (CookFlags & InCookFlags) != ECookInitializationFlags::None;
}


void InitializeTargetPlatforms(const TArrayView<ITargetPlatform* const>& NewTargetPlatforms)
{
	//allow each platform to update its internals before cooking
	for (ITargetPlatform* TargetPlatform : NewTargetPlatforms)
	{
		TargetPlatform->RefreshSettings();
	}
}


bool ContainsRedirector(const FName& PackageName, TMap<FName, FName>& RedirectedPaths)
{
	bool bFoundRedirector = false;
	TArray<FAssetData> Assets;
	// ensure(AssetRegistry->GetAssetsByPackageName(PackageName, Assets, true));

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.IsRedirector())
		{
			FName RedirectedPath;
			FString RedirectedPathString;
			if (Asset.GetTagValue("DestinationObject", RedirectedPathString))
			{
				ConstructorHelpers::StripObjectClass(RedirectedPathString);
				RedirectedPath = FName(*RedirectedPathString);
				FAssetData DestinationData = GetAssetRegistry()->GetAssetByObjectPath(RedirectedPath, true);
				TSet<FName> SeenPaths;

				SeenPaths.Add(RedirectedPath);

				// Need to follow chain of redirectors
				while (DestinationData.IsRedirector())
				{
					if (DestinationData.GetTagValue("DestinationObject", RedirectedPathString))
					{
						ConstructorHelpers::StripObjectClass(RedirectedPathString);
						RedirectedPath = FName(*RedirectedPathString);

						if (SeenPaths.Contains(RedirectedPath))
						{
							// Recursive, bail
							DestinationData = FAssetData();
						}
						else
						{
							SeenPaths.Add(RedirectedPath);
							DestinationData = GetAssetRegistry()->GetAssetByObjectPath(RedirectedPath, true);
						}
					}
					else
					{
						// Can't extract
						DestinationData = FAssetData();						
					}
				}

				// DestinationData may be invalid if this is a subobject, check package as well
				bool bDestinationValid = DestinationData.IsValid();

				if (!bDestinationValid)
				{
					// we can;t call GetCachedStandardPackageFileFName with None
					if (RedirectedPath != NAME_None)
					{
						FName StandardPackageName = FName(*FPackageName::ObjectPathToPackageName(RedirectedPathString));
						if (StandardPackageName != NAME_None)
						{
							bDestinationValid = true;
						}
					}
				}

				if (bDestinationValid)
				{
					RedirectedPaths.Add(Asset.ObjectPath, RedirectedPath);
				}
				else
				{
					RedirectedPaths.Add(Asset.ObjectPath, NAME_None);
					// UE_LOG(LogCook, Log, TEXT("Found redirector in package %s pointing to deleted object %s"), *PackageName.ToString(), *RedirectedPathString);
				}

				bFoundRedirector = true;
			}
		}
	}
	return bFoundRedirector;
}

#include "Editor.h"
void UPackageAssetsCollector::CollectFilesToCook(TArray<FName>& FilesInPath, const TArray<FString>& CookMaps, const TArray<FString>& InCookDirectories,
	const TArray<FString> &IniMapSections, ECookByTheBookOptions FilesToCookFlags, const TArray<ITargetPlatform*>& InTargetPlatforms)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UCookOnTheFlyServer::CollectFilesToCook);

// #if OUTPUT_TIMING
// 	SCOPE_TIMER(CollectFilesToCook);
// #endif
	UProjectPackagingSettings* PackagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());

	bool bCookAll = (!!(FilesToCookFlags & ECookByTheBookOptions::CookAll)) || PackagingSettings->bCookAll;
	bool bMapsOnly = (!!(FilesToCookFlags & ECookByTheBookOptions::MapsOnly)) || PackagingSettings->bCookMapsOnly;
	bool bNoDev = !!(FilesToCookFlags & ECookByTheBookOptions::NoDevContent);

	TArray<FName> InitialPackages = FilesInPath;


	TArray<FString> CookDirectories = InCookDirectories;
	
	if (!(FilesToCookFlags & ECookByTheBookOptions::NoAlwaysCookMaps))
	{

		{
			TArray<FString> MapList;
			// Add the default map section
			GEditor->LoadMapListFromIni(TEXT("AlwaysCookMaps"), MapList);

			for (int32 MapIdx = 0; MapIdx < MapList.Num(); MapIdx++)
			{
				// UE_LOG(LogCook, Verbose, TEXT("Maplist contains has %s "), *MapList[MapIdx]);
				AddFileToCook(FilesInPath, MapList[MapIdx]);
			}
		}


		bool bFoundMapsToCook = CookMaps.Num() > 0;

		{
			TArray<FString> MapList;
			for (const auto& IniMapSection : IniMapSections)
			{
				// UE_LOG(LogCook, Verbose, TEXT("Loading map ini section %s "), *IniMapSection);
				GEditor->LoadMapListFromIni(*IniMapSection, MapList);
			}
			for (int32 MapIdx = 0; MapIdx < MapList.Num(); MapIdx++)
			{
				// UE_LOG(LogCook, Verbose, TEXT("Maplist contains has %s "), *MapList[MapIdx]);
				AddFileToCook(FilesInPath, MapList[MapIdx]);
				bFoundMapsToCook = true;
			}
		}

		// If we didn't find any maps look in the project settings for maps
		for (const FFilePath& MapToCook : PackagingSettings->MapsToCook)
		{
			// UE_LOG(LogCook, Verbose, TEXT("Maps to cook list contains %s "), *MapToCook.FilePath);
			FilesInPath.Add(FName(*MapToCook.FilePath));
			bFoundMapsToCook = true;
		}



		// if we didn't find maps to cook, and we don't have any commandline maps (CookMaps), then cook the allmaps section
		if (bFoundMapsToCook == false && CookMaps.Num() == 0)
		{
			// UE_LOG(LogCook, Verbose, TEXT("Loading default map ini section AllMaps "));
			TArray<FString> AllMapsSection;
			GEditor->LoadMapListFromIni(TEXT("AllMaps"), AllMapsSection);
			for (const FString& MapName : AllMapsSection)
			{
				AddFileToCook(FilesInPath, MapName);
			}
		}

		// Also append any cookdirs from the project ini files; these dirs are relative to the game content directory or start with a / root
		{
			const FString AbsoluteGameContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
			for (const FDirectoryPath& DirToCook : PackagingSettings->DirectoriesToAlwaysCook)
			{
				// UE_LOG(LogCook, Verbose, TEXT("Loading directory to always cook %s"), *DirToCook.Path);

				if (DirToCook.Path.StartsWith(TEXT("/"), ESearchCase::CaseSensitive))
				{
					// If this starts with /, this includes a root like /engine
					FString RelativePath = FPackageName::LongPackageNameToFilename(DirToCook.Path / TEXT(""));
					CookDirectories.Add(FPaths::ConvertRelativePathToFull(RelativePath));
				}
				else
				{
					// This is relative to /game
					CookDirectories.Add(AbsoluteGameContentDir / DirToCook.Path);
				}
			}
		}
	}
	if (!(FilesToCookFlags & ECookByTheBookOptions::NoGameAlwaysCookPackages))
	{
		// COOK_STAT(FScopedDurationTimer TickTimer(DetailedCookStats::GameCookModificationDelegateTimeSec));
		// SCOPE_TIMER(CookModificationDelegate);
#define DEBUG_COOKMODIFICATIONDELEGATE 0
#if DEBUG_COOKMODIFICATIONDELEGATE
		TSet<UPackage*> LoadedPackages;
		for ( TObjectIterator<UPackage> It; It; ++It)
		{
			LoadedPackages.Add(*It);
		}
#endif

		// allow the game to fill out the asset registry, as well as get a list of objects to always cook
		TArray<FString> FilesInPathStrings;
		FGameDelegates::Get().GetCookModificationDelegate().ExecuteIfBound(FilesInPathStrings);

		for (const FString& FileString : FilesInPathStrings)
		{
			FilesInPath.Add(FName(*FileString));
		}

		if (UAssetManager::IsValid())
		{
			TArray<FName> PackagesToNeverCook;

			UAssetManager::Get().ModifyCook(FilesInPath, PackagesToNeverCook);

			for (FName NeverCookPackage : PackagesToNeverCook)
			{
				const FName StandardPackageFilename = NeverCookPackage;// PackageNameCache->GetCachedStandardPackageFileFName(NeverCookPackage);

				if (StandardPackageFilename != NAME_None)
				{
					CookTracker.NeverCookPackageList.Add(StandardPackageFilename);
				}
			}
		}
#if DEBUG_COOKMODIFICATIONDELEGATE
		for (TObjectIterator<UPackage> It; It; ++It)
		{
			if ( !LoadedPackages.Contains(*It) )
			{
				// UE_LOG(LogCook, Display, TEXT("CookModificationDelegate loaded %s"), *It->GetName());
			}
		}
#endif

		// if (UE_LOG_ACTIVE(LogCook, Verbose) )
		// {
		// 	for ( const FString& FileName : FilesInPathStrings )
		// 	{
		// 		// UE_LOG(LogCook, Verbose, TEXT("Cook modification delegate requested package %s"), *FileName);
		// 	}
		// }
	}

	for ( const FString& CurrEntry : CookMaps )
	{
		// SCOPE_TIMER(SearchForPackageOnDisk);
		if (FPackageName::IsShortPackageName(CurrEntry))
		{
			FString OutFilename;
			if (FPackageName::SearchForPackageOnDisk(CurrEntry, NULL, &OutFilename) == false)
			{
				// LogCookerMessage( FString::Printf(TEXT("Unable to find package for map %s."), *CurrEntry), EMessageSeverity::Warning);
				// UE_LOG(LogCook, Warning, TEXT("Unable to find package for map %s."), *CurrEntry);
			}
			else
			{
				AddFileToCook( FilesInPath, OutFilename);
			}
		}
		else
		{
			AddFileToCook( FilesInPath,CurrEntry);
		}
	}



	const FString ExternalMountPointName(TEXT("/Game/"));
	// if (IsCookingDLC())
	// {
	// 	// get the dlc and make sure we cook that directory 
	// 	FString DLCPath = FPaths::Combine(*GetBaseDirectoryForDLC(), TEXT("Content"));
	//
	// 	TArray<FString> Files;
	// 	IFileManager::Get().FindFilesRecursive(Files, *DLCPath, *(FString(TEXT("*")) + FPackageName::GetAssetPackageExtension()), true, false, false);
	// 	IFileManager::Get().FindFilesRecursive(Files, *DLCPath, *(FString(TEXT("*")) + FPackageName::GetMapPackageExtension()), true, false, false);
	// 	for (int32 Index = 0; Index < Files.Num(); Index++)
	// 	{
	// 		FString StdFile = Files[Index];
	// 		FPaths::MakeStandardFilename(StdFile);
	// 		AddFileToCook(FilesInPath, StdFile);
	//
	// 		// this asset may not be in our currently mounted content directories, so try to mount a new one now
	// 		FString LongPackageName;
	// 		if (!FPackageName::IsValidLongPackageName(StdFile) && !FPackageName::TryConvertFilenameToLongPackageName(StdFile, LongPackageName))
	// 		{
	// 			FPackageName::RegisterMountPoint(ExternalMountPointName, DLCPath);
	// 		}
	// 	}
	// }


	if (!(FilesToCookFlags & ECookByTheBookOptions::DisableUnsolicitedPackages))
	{
		for (const FString& CurrEntry : CookDirectories)
		{
			TArray<FString> Files;
			IFileManager::Get().FindFilesRecursive(Files, *CurrEntry, *(FString(TEXT("*")) + FPackageName::GetAssetPackageExtension()), true, false);
			for (int32 Index = 0; Index < Files.Num(); Index++)
			{
				FString StdFile = Files[Index];
				FPaths::MakeStandardFilename(StdFile);
				AddFileToCook(FilesInPath, StdFile);

				// this asset may not be in our currently mounted content directories, so try to mount a new one now
				FString LongPackageName;
				if (!FPackageName::IsValidLongPackageName(StdFile) && !FPackageName::TryConvertFilenameToLongPackageName(StdFile, LongPackageName))
				{
					FPackageName::RegisterMountPoint(ExternalMountPointName, CurrEntry);
				}
			}
		}

		// If no packages were explicitly added by command line or game callback, add all maps
		if (FilesInPath.Num() == InitialPackages.Num() || bCookAll)
		{
			TArray<FString> Tokens;
			Tokens.Empty(2);
			Tokens.Add(FString("*") + FPackageName::GetAssetPackageExtension());
			Tokens.Add(FString("*") + FPackageName::GetMapPackageExtension());

			uint8 PackageFilter = NORMALIZE_DefaultFlags | NORMALIZE_ExcludeEnginePackages | NORMALIZE_ExcludeLocalizedPackages;
			if (bMapsOnly)
			{
				PackageFilter |= NORMALIZE_ExcludeContentPackages;
			}

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
					// UE_LOG(LogCook, Display, TEXT("No packages found for parameter %i: '%s'"), TokenIndex, *Tokens[TokenIndex]);
					continue;
				}

				for (int32 TokenFileIndex = 0; TokenFileIndex < TokenFiles.Num(); ++TokenFileIndex)
				{
					AddFileToCook(FilesInPath, TokenFiles[TokenFileIndex]);
				}
			}
		}
	}

	if (!(FilesToCookFlags & ECookByTheBookOptions::NoDefaultMaps))
	{
		// make sure we cook the default maps
		// Collect the default maps for all requested platforms.  Our additions are potentially wasteful if different platforms in the requested list have different default maps.
		// In that case we will wastefully cook maps for platforms that don't require them.
		for (const ITargetPlatform* TargetPlatform : InTargetPlatforms)
		{
			// load the platform specific ini to get its DefaultMap
			FConfigFile PlatformEngineIni;
			FConfigCacheIni::LoadLocalIniFile(PlatformEngineIni, TEXT("Engine"), true, *TargetPlatform->IniPlatformName());

			// get the server and game default maps and cook them
			FString Obj;
			if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GameDefaultMap"), Obj))
			{
				if (Obj != FName(NAME_None).ToString())
				{
					AddFileToCook(FilesInPath, Obj);
				}
			}
			if (IsCookFlagSet(ECookInitializationFlags::IncludeServerMaps))
			{
				if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("ServerDefaultMap"), Obj))
				{
					if (Obj != FName(NAME_None).ToString())
					{
						AddFileToCook(FilesInPath, Obj);
					}
				}
			}
			if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GlobalDefaultGameMode"), Obj))
			{
				if (Obj != FName(NAME_None).ToString())
				{
					AddFileToCook(FilesInPath, Obj);
				}
			}
			if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GlobalDefaultServerGameMode"), Obj))
			{
				if (Obj != FName(NAME_None).ToString())
				{
					AddFileToCook(FilesInPath, Obj);
				}
			}
			if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GameInstanceClass"), Obj))
			{
				if (Obj != FName(NAME_None).ToString())
				{
					AddFileToCook(FilesInPath, Obj);
				}
			}
		}
	}

	if (!(FilesToCookFlags & ECookByTheBookOptions::NoInputPackages))
	{
		// make sure we cook any extra assets for the default touch interface
		// @todo need a better approach to cooking assets which are dynamically loaded by engine code based on settings
		FConfigFile InputIni;
		FString InterfaceFile;
		FConfigCacheIni::LoadLocalIniFile(InputIni, TEXT("Input"), true);
		if (InputIni.GetString(TEXT("/Script/Engine.InputSettings"), TEXT("DefaultTouchInterface"), InterfaceFile))
		{
			if (InterfaceFile != TEXT("None") && InterfaceFile != TEXT(""))
			{
				AddFileToCook(FilesInPath, InterfaceFile);
			}
		}
	}
	//@todo SLATE: This is a hack to ensure all slate referenced assets get cooked.
	// Slate needs to be refactored to properly identify required assets at cook time.
	// Simply jamming everything in a given directory into the cook list is error-prone
	// on many levels - assets not required getting cooked/shipped; assets not put under 
	// the correct folder; etc.
	if ( !(FilesToCookFlags & ECookByTheBookOptions::NoSlatePackages))
	{
		TArray<FString> UIContentPaths;
		TSet <FName> ContentDirectoryAssets; 
		if (GConfig->GetArray(TEXT("UI"), TEXT("ContentDirectories"), UIContentPaths, GEditorIni) > 0)
		{
			for (int32 DirIdx = 0; DirIdx < UIContentPaths.Num(); DirIdx++)
			{
				FString ContentPath = FPackageName::LongPackageNameToFilename(UIContentPaths[DirIdx]);

				TArray<FString> Files;
				IFileManager::Get().FindFilesRecursive(Files, *ContentPath, *(FString(TEXT("*")) + FPackageName::GetAssetPackageExtension()), true, false);
				for (int32 Index = 0; Index < Files.Num(); Index++)
				{
					FString StdFile = Files[Index];
					FName PackageName = FName(*FPackageName::FilenameToLongPackageName(StdFile));
					ContentDirectoryAssets.Add(PackageName);
					FPaths::MakeStandardFilename(StdFile);
					AddFileToCook( FilesInPath, StdFile);
				}
			}
		}

		if (CookByTheBookOptions.bGenerateDependenciesForMaps) 
		{
			for (auto& MapDependencyGraph : CookByTheBookOptions.MapDependencyGraphs)
			{
				MapDependencyGraph.Value.Add(FName(TEXT("ContentDirectoryAssets")), ContentDirectoryAssets);
			}
		}
	}

	if (!(FilesToCookFlags & ECookByTheBookOptions::DisableUnsolicitedPackages))
	{
		// Gather initial unsolicited package list, this is needed in iterative mode because all explicitly requested packages may have already been cooked
		// and so the code inside the TIckCookOnTheSide build loop might never run and never get a chance to call GetUnsolicitedPackages
		// UE_LOG(LogCook, Verbose, TEXT("Finding initial unsolicited packages"));

		TArray<UPackage*> UnsolicitedPackages = GetUnsolicitedPackages(InTargetPlatforms);
		
		for (UPackage* UnsolicitedPackage : UnsolicitedPackages)
		{
			AddFileToCook(FilesInPath, UnsolicitedPackage->GetName());
		}
	}
	
}



void UPackageAssetsCollector::AddFileToCook( TArray<FName>& InOutFilesToCook, const FString &InFilename ) const
{ 
	if (!FPackageName::IsScriptPackage(InFilename) && !FPackageName::IsMemoryPackage(InFilename))
	{
		FName InFilenameName = FName(*InFilename );
		if ( InFilenameName == NAME_None)
		{
			return;
		}

		InOutFilesToCook.AddUnique(InFilenameName);
	}
}

TArray<FString> UPackageAssetsCollector::StartAssetsCollector(const TArray<FString>& PlatformNames)
{
	
	FCookByTheBookStartupOptions StartupOptions;
	TArray<ITargetPlatform*> ActiveTargetPlatforms;
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	ActiveTargetPlatforms = TPM.GetActiveTargetPlatforms();

	TArray<ITargetPlatform*> CollectorPlatforms;
	for(const auto& PlatformName:PlatformNames)
	{
		for(const auto& PlatfomIns:ActiveTargetPlatforms)
		{
			if(PlatfomIns->PlatformName().Equals(PlatformName))
			{
				CollectorPlatforms.Emplace(PlatfomIns);
				break;
			}
		}
	}
	StartupOptions.CookOptions = ECookByTheBookOptions::NoAlwaysCookMaps | ECookByTheBookOptions::NoDefaultMaps | ECookByTheBookOptions::NoGameAlwaysCookPackages | ECookByTheBookOptions::NoInputPackages | ECookByTheBookOptions::NoSlatePackages | ECookByTheBookOptions::DisableUnsolicitedPackages | ECookByTheBookOptions::ForceDisableSaveGlobalShaders;
	
	return AssetsCollector(StartupOptions,CollectorPlatforms);
}

inline TArray<FString> UPackageAssetsCollector::AssetsCollector(
	const FCookByTheBookStartupOptions& CookByTheBookStartupOptions, const TArray<ITargetPlatform*> InTargetPlatforms)
{
	PackageTracker = MakeShareable(new FPackageTracker);
	// mTargetPlatforms = InTargetPlatforms;
	const TArray<FString>& CookMaps = CookByTheBookStartupOptions.CookMaps;
	const TArray<FString>& CookDirectories = CookByTheBookStartupOptions.CookDirectories;
	const TArray<FString>& IniMapSections = CookByTheBookStartupOptions.IniMapSections;
	const ECookByTheBookOptions& CookOptions = CookByTheBookStartupOptions.CookOptions;
	const FString& DLCName = CookByTheBookStartupOptions.DLCName;

	const FString& CreateReleaseVersion = CookByTheBookStartupOptions.CreateReleaseVersion;
	const FString& BasedOnReleaseVersion = CookByTheBookStartupOptions.BasedOnReleaseVersion;
	
	// GenerateAssetRegistry();

	// SelectSessionPlatforms does not check for uniqueness and non-null, and we rely on those properties for performance, so ensure it here before calling SelectSessionPlatforms
	TArray<ITargetPlatform*> TargetPlatforms;
	TargetPlatforms.Reserve(CookByTheBookStartupOptions.TargetPlatforms.Num());
	for (ITargetPlatform* TargetPlatform : CookByTheBookStartupOptions.TargetPlatforms)
	{
		if (TargetPlatform)
		{
			TargetPlatforms.AddUnique(TargetPlatform);
		}
	}

	IAssetRegistry* AssetRegistry = GetAssetRegistry();
	
	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
		// Find all the localized packages and map them back to their source package
	{
		TArray<FString> AllCulturesToCook = CookByTheBookStartupOptions.CookCultures;
		for (const FString& CultureName : CookByTheBookStartupOptions.CookCultures)
		{
			const TArray<FString> PrioritizedCultureNames = FInternationalization::Get().GetPrioritizedCultureNames(CultureName);
			for (const FString& PrioritizedCultureName : PrioritizedCultureNames)
			{
				AllCulturesToCook.AddUnique(PrioritizedCultureName);
			}
		}
		AllCulturesToCook.Sort();

		// // UE_LOG(LogCook, Display, TEXT("Discovering localized assets for cultures: %s"), *FString::Join(AllCulturesToCook, TEXT(", ")));

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

			TArray<FName>& LocalizedPackageNames = CookByTheBookOptions.SourceToLocalizedPackageVariants.FindOrAdd(SourcePackageName);
			LocalizedPackageNames.AddUnique(LocalizedPackageName);
		}

		// Get the list of localization targets to chunk, and remove any targets that we've been asked not to stage
		TArray<FString> LocalizationTargetsToChunk = PackagingSettings->LocalizationTargetsToChunk;
		{
			TArray<FString> BlacklistLocalizationTargets;
			GConfig->GetArray(TEXT("Staging"), TEXT("BlacklistLocalizationTargets"), BlacklistLocalizationTargets, GGameIni);
			if (BlacklistLocalizationTargets.Num() > 0)
			{
				LocalizationTargetsToChunk.RemoveAll([&BlacklistLocalizationTargets](const FString& InLocalizationTarget)
				{
					return BlacklistLocalizationTargets.Contains(InLocalizationTarget);
				});
			}
		}

		// if (LocalizationTargetsToChunk.Num() > 0 && AllCulturesToCook.Num() > 0)
		// {
		// 	for (const ITargetPlatform* TargetPlatform : TargetPlatforms)
		// 	{
		// 		FAssetRegistryGenerator* RegistryGenerator = PlatformManager->GetPlatformData(TargetPlatform)->RegistryGenerator.Get();
		// 		RegistryGenerator->RegisterChunkDataGenerator(MakeShared<FLocalizationChunkDataGenerator>(PackagingSettings->LocalizationTargetCatchAllChunkId, LocalizationTargetsToChunk, AllCulturesToCook));
		// 	}
		// }
	}

	{
		const FString AbsoluteGameContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

		TArray<FString> NeverCookDirectories = CookByTheBookStartupOptions.NeverCookDirectories;

		for (const FDirectoryPath& DirToNotCook : PackagingSettings->DirectoriesToNeverCook)
		{
			if (DirToNotCook.Path.StartsWith(TEXT("/"), ESearchCase::CaseSensitive))
			{
				// If this starts with /, this includes a root like /engine
				FString RelativePath = FPackageName::LongPackageNameToFilename(DirToNotCook.Path / TEXT(""));
				NeverCookDirectories.Add(FPaths::ConvertRelativePathToFull(RelativePath));
			}
			else
			{
				// This is relative to /game
				NeverCookDirectories.Add(AbsoluteGameContentDir / DirToNotCook.Path);
			}

		}

		for (const FString& NeverCookDirectory : NeverCookDirectories)
		{
			// add the packages to the never cook package list
			struct FNeverCookDirectoryWalker : public IPlatformFile::FDirectoryVisitor
			{
			private:
				FThreadSafeSet<FName>& NeverCookPackageList;
			public:
				FNeverCookDirectoryWalker(FThreadSafeSet<FName> &InNeverCookPackageList) : NeverCookPackageList(InNeverCookPackageList) { }

				// IPlatformFile::FDirectoryVisitor interface
				virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
				{
					if (bIsDirectory)
					{
						return true;
					}
					FString StandardFilename = FString(FilenameOrDirectory);
					FPaths::MakeStandardFilename(StandardFilename);

					NeverCookPackageList.Add(FName(*StandardFilename));
					return true;
				}

			} NeverCookDirectoryWalker(CookTracker.NeverCookPackageList);

			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

			PlatformFile.IterateDirectoryRecursively(*NeverCookDirectory, NeverCookDirectoryWalker);
		}

	}

	// use temp list of UBT platform strings to discover PlatformSpecificNeverCookPackages
	{
		TArray<FString> UBTPlatformStrings;
		UBTPlatformStrings.Reserve(TargetPlatforms.Num());
		for (const ITargetPlatform* Platform : TargetPlatforms)
		{
			FString UBTPlatformName;
			Platform->GetPlatformInfo().UBTTargetId.ToString(UBTPlatformName);
			UBTPlatformStrings.Emplace(MoveTemp(UBTPlatformName));
		}

		DiscoverPlatformSpecificNeverCookPackages(TargetPlatforms, UBTPlatformStrings);
	}

	{
		if (CookByTheBookOptions.bGenerateDependenciesForMaps)
		{
			for (const ITargetPlatform* Platform : TargetPlatforms)
			{
				CookByTheBookOptions.MapDependencyGraphs.Add(Platform);
			}
		}
	}

	TArray<FName> FilesInPath;
	TSet<FName> StartupSoftObjectPackages;

	// Get the list of soft references, for both empty package and all startup packages
	GRedirectCollector.ProcessSoftObjectPathPackageList(NAME_None, false, StartupSoftObjectPackages);

	for (const FName& StartupPackage : CookByTheBookOptions.StartupPackages)
	{
		GRedirectCollector.ProcessSoftObjectPathPackageList(StartupPackage, false, StartupSoftObjectPackages);
	}
	CollectFilesToCook(FilesInPath, CookMaps, CookDirectories, IniMapSections, CookOptions, InTargetPlatforms);

	// Add string asset packages after collecting files, to avoid accidentally activating the behavior to cook all maps if none are specified
	for (FName SoftObjectPackage : StartupSoftObjectPackages)
	{
		TMap<FName, FName> RedirectedPaths;

		// If this is a redirector, extract destination from asset registry
		if (ContainsRedirector(SoftObjectPackage, RedirectedPaths))
		{
			for (TPair<FName, FName>& RedirectedPath : RedirectedPaths)
			{
				GRedirectCollector.AddAssetPathRedirection(RedirectedPath.Key, RedirectedPath.Value);
			}
		}

		if (!CookByTheBookOptions.bDisableUnsolicitedPackages)
		{
			AddFileToCook(FilesInPath, SoftObjectPackage.ToString());
		}
	}

	TArray<FString> result;
	for(const auto& File:FilesInPath)
	{
		result.Add(File.ToString());
		UE_LOG(LogTemp,Log,TEXT("Cook File: %s"),*File.ToString());
	}
	return result;
}


TArray<UPackage*> UPackageAssetsCollector::GetUnsolicitedPackages(const TArray<ITargetPlatform*>& TargetPlatforms) const
{
	// SCOPE_TIMER(GeneratePackageNames);

	TArray<UPackage*> PackagesToSave;

	for (UPackage* Package : PackageTracker->LoadedPackages)
	{
		check(Package != nullptr);

		const FName StandardPackageFName = *Package->GetName();//PackageNameCache->GetCachedStandardPackageFileFName(Package);

		if (StandardPackageFName == NAME_None)
			continue;	// if we have name none that means we are in core packages or something...

		// if (PackageTracker->CookedPackages.Exists(StandardPackageFName, TargetPlatforms))
		// 	continue;

		PackagesToSave.Add(Package);

		// UE_LOG(LogCook, Verbose, TEXT("Found unsolicited package to cook '%s'"), *Package->GetName());
	}

	return PackagesToSave;
}