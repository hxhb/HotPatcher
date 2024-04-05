#include "DependenciesParser/FDefaultAssetDependenciesParser.h"
#include "FlibAssetManageHelper.h"
#include "HotPatcherLog.h"
#include "HotPatcherRuntime.h"
#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "Engine/WorldComposition.h"
#include "Resources/Version.h"
#include "Misc/EngineVersionComparison.h"

void FAssetDependenciesParser::Parse(const FAssetDependencies& InParseConfig)
{
	ParseConfig = InParseConfig;
	
	SCOPED_NAMED_EVENT_TEXT("FAssetDependenciesParser::Parse",FColor::Red);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TSet<FName> AssetPackageNames;
	
	// make /Game/XXX to /Game/XXX/
	TArray<FString> IngnoreFilters = UFlibAssetManageHelper::NormalizeContentDirs(InParseConfig.IgnoreFilters);
	TArray<FString> IncludeFilters = UFlibAssetManageHelper::NormalizeContentDirs(InParseConfig.IncludeFilters);
	
	{
		SCOPED_NAMED_EVENT_TEXT("Get AssetPackageNames by filter",FColor::Red);
		TArray<FAssetData> AssetDatas;
		UFlibAssetManageHelper::GetAssetsData(IncludeFilters,AssetDatas,!GForceSingleThread);

		for(const auto& AssetData:AssetDatas)
		{
			if(!IsIgnoreAsset(AssetData))
			{
				AssetPackageNames.Add(AssetData.PackageName);
			}
		}
	}
	
	Results.Append(AssetPackageNames);

	if(InParseConfig.AnalysicFilterDependencies)
	{
		SCOPED_NAMED_EVENT_TEXT("Get dependencies for filters",FColor::Red);
		for(FName PackageName:AssetPackageNames)
		{
			Results.Append(
				FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(
					AssetRegistryModule,
					PackageName,
					ParseConfig.AssetRegistryDependencyTypes,
					true,
					IngnoreFilters,
					InParseConfig.ForceSkipContents,
					InParseConfig.ForceSkipPackageNames,
					ParseConfig.IgnoreAseetTypes,
					ScanedCaches
					));
		}
	}
	
	{
		SCOPED_NAMED_EVENT_TEXT("Get dependencies for SpecifyAsset",FColor::Red);
		for(const auto& SpecifyAsset:ParseConfig.InIncludeSpecifyAsset)
		{
			FString LongPackageName = SpecifyAsset.Asset.GetLongPackageName();
			if(!SpecifyAsset.Asset.IsValid())
			{
				continue;
			}
			FAssetData AssetData;
			if(!LongPackageName.IsEmpty() && UFlibAssetManageHelper::GetAssetsDataByPackageName(LongPackageName,AssetData))
			{
				if(IsIgnoreAsset(AssetData))
				{
					continue;
				}
			}
			Results.Add(FName(*SpecifyAsset.Asset.GetLongPackageName()));
			if(SpecifyAsset.bAnalysisAssetDependencies)
			{
				Results.Append(
					FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(
						AssetRegistryModule,
						FName(*LongPackageName),
						SpecifyAsset.AssetRegistryDependencyTypes,
						SpecifyAsset.bAnalysisAssetDependencies,
						IngnoreFilters,
						InParseConfig.ForceSkipContents,
						InParseConfig.ForceSkipPackageNames,
						ParseConfig.IgnoreAseetTypes,
						ScanedCaches
						));
			}
		}
	}

	Results.Remove(FName(NAME_None));

	// FCriticalSection	SynchronizationObject;
	// TSet<FName> ForceSkipPackages;
	// TArray<FName> ResultArray = Results.Array();
	// ParallelFor(ResultArray.Num(),[&](int32 index)
	// {
	// 	FString AssetPackageNameStr = ResultArray[index].ToString();
	// 	if(IsForceSkipAsset(AssetPackageNameStr,ParseConfig.IgnoreAseetTypes,ParseConfig.ForceSkipContents))
	// 	{
	// 		FScopeLock Lock(&SynchronizationObject);
	// 		ForceSkipPackages.Add(ResultArray[index]);
	// 	}
	// },GForceSingleThread);
	//
	// for(FName SkipPackage:ForceSkipPackages)
	// {
	// 	Results.Remove(SkipPackage);
	// }
}

bool IsValidPackageName(const FString& LongPackageName)
{
	SCOPED_NAMED_EVENT_TEXT("IsValidPackageName",FColor::Red);
	bool bStatus = false;
	if (!LongPackageName.IsEmpty() && !FPackageName::IsScriptPackage(LongPackageName) && !FPackageName::IsMemoryPackage(LongPackageName))
	{
		bStatus = true;
	}
	return bStatus;
}

bool FAssetDependenciesParser::IsIgnoreAsset(const FAssetData& AssetData)
{
	FString LongPackageName = AssetData.PackageName.ToString();
	bool bIsForceSkip = IsForceSkipAsset(LongPackageName,ParseConfig.IgnoreAseetTypes,ParseConfig.IgnoreFilters,ParseConfig.ForceSkipContents,ParseConfig.ForceSkipPackageNames,true);
	auto HashPackageFlag = [](uint32 Flags,uint32 CheckFlag)->bool
	{
		return (Flags & CheckFlag) != 0;	
	};
	bool bIsEditorFlag = HashPackageFlag(AssetData.PackageFlags,PKG_EditorOnly);
	
	return bIsForceSkip || bIsEditorFlag;
}

bool FAssetDependenciesParser::IsForceSkipAsset(const FString& LongPackageName, const TSet<FName>& IgnoreTypes, const TArray<FString>& IgnoreFilters, TArray<FString> ForceSkipFilters, const
                                                TSet<FString>& ForceSkipPackageNames, bool
                                                bDispalyLog)
{
	SCOPED_NAMED_EVENT_TEXT("IsForceSkipAsset",FColor::Red);
	bool bIsIgnore = false;
	FString MatchIgnoreStr;
	if(UFlibAssetManageHelper::UFlibAssetManageHelper::MatchIgnoreTypes(LongPackageName,IgnoreTypes,MatchIgnoreStr))
	{
		bIsIgnore = true;
	}
	
	if(!bIsIgnore && UFlibAssetManageHelper::MatchIgnoreFilters(LongPackageName,IgnoreFilters,MatchIgnoreStr))
	{
		bIsIgnore = true;
	}

	if(!bIsIgnore && UFlibAssetManageHelper::MatchIgnoreFilters(LongPackageName,ForceSkipFilters,MatchIgnoreStr))
	{
		bIsIgnore = true;
	}

	if(!bIsIgnore && ForceSkipPackageNames.Contains(LongPackageName))
	{
		bIsIgnore = true;
	}

	if(bIsIgnore && bDispalyLog)
	{
#if ASSET_DEPENDENCIES_DEBUG_LOG
		UE_LOG(LogHotPatcher,Log,TEXT("Force Skip %s (match ignore rule %s)"),*LongPackageName,*MatchIgnoreStr);
#endif
	}
	return bIsIgnore;
}

TArray<FName> ParserSkipAssetByDependencies(const FAssetData& CurrentAssetData,const TArray<FName>& CurrentAssetDependencies)
{
	SCOPED_NAMED_EVENT_TEXT("ParserSkipAssetByDependencies",FColor::Red);
	bool bHasAtlsGroup = false;
	TArray<FName> TextureSrouceRefs;
	if(UFlibAssetManageHelper::GetAssetDataClasses(CurrentAssetData).IsEqual(TEXT("PaperSprite")))
	{
		for(const auto& DependItemName:CurrentAssetDependencies)
		{
			FAssetDetail AssetDetail = UFlibAssetManageHelper::GetAssetDetailByPackageName(DependItemName.ToString());
			if(AssetDetail.IsValid())
			{
				if(AssetDetail.AssetType.IsEqual(TEXT("PaperSpriteAtlas")))
				{
					bHasAtlsGroup = true;
				}
				if(AssetDetail.AssetType.IsEqual(TEXT("Texture")) || AssetDetail.AssetType.IsEqual(TEXT("Texture2D")))
				{
					TextureSrouceRefs.Add(DependItemName);
				}
			}
		}
	}
	return bHasAtlsGroup ? TextureSrouceRefs : TArray<FName>{};
}

TSet<FName> FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(
	FAssetRegistryModule& InAssetRegistryModule,
	FName InLongPackageName, const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
    bool bRecursively,
    const TArray<FString>& IgnoreDirectories,
    const TArray<FString>& ForceSkipDirectories,
    const TSet<FString>& ForceSkipPackageNames,
    const TSet<FName>& IgnoreAssetTypes,
    FScanedCachesType& InScanedCaches)
{
	static bool bVerboseLog = FParse::Param(FCommandLine::Get(), TEXT("VerboseLog"));
	
	TSet<FName> AssetDependencies;
	SCOPED_NAMED_EVENT_TEXT("GatherAssetDependicesInfoRecursively",FColor::Red);
	TSet<FString> TempForceSkipPackageNames = ForceSkipPackageNames;
	TempForceSkipPackageNames.Add(InLongPackageName.ToString());

	FAssetData CurrentAssetData;
	UFlibAssetManageHelper::GetAssetsDataByPackageName(InLongPackageName.ToString(),CurrentAssetData);

	bool bGetDependenciesSuccess = false;
	EAssetRegistryDependencyType::Type TotalType = EAssetRegistryDependencyType::None;

	for (const auto& DepType : InAssetDependencyTypes)
	{
		TotalType = UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(DepType);
	}

	TArray<FName> CurrentAssetDependencies;
	
	{
		SCOPED_NAMED_EVENT_TEXT("GetDependencies",FColor::Red);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			bGetDependenciesSuccess = InAssetRegistryModule.Get().GetDependencies(InLongPackageName, CurrentAssetDependencies,

#if UE_VERSION_OLDER_THAN(5,3,0)
				TotalType
#else
				UE::AssetRegistry::EDependencyCategory::Package
#endif
		);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		for(const auto& SkipForDependencies:ParserSkipAssetByDependencies(CurrentAssetData,CurrentAssetDependencies))
		{
			CurrentAssetDependencies.Remove(SkipForDependencies);
		}
		
		// collect world composition tile packages to cook 
		if(ParseConfig.bSupportWorldComposition && CurrentAssetData.GetClass() == UWorld::StaticClass())
		{
#if WITH_EDITOR
			FString DisplayStr = FString::Printf(TEXT("LoadWorld %s"),*CurrentAssetData.GetFullName());
			FScopedNamedEvent CacheClassEvent(FColor::Red,*DisplayStr);
			UWorld* World = UWorld::FindWorldInPackage(CurrentAssetData.GetAsset()->GetOutermost());
			if (World)
			{
				if(World->WorldComposition)
				{
					SCOPED_NAMED_EVENT_TEXT("Collect WorldComposition Tiles",FColor::Red);
					TArray<FString> PackageNames;
					World->WorldComposition->CollectTilesToCook(PackageNames);
					for(const auto& PackageName:PackageNames)
					{
#if ASSET_DEPENDENCIES_DEBUG_LOG
						UE_LOG(LogHotPatcher,Log,TEXT("Collecting WorldComposition Tile Package %s for %s"),*InLongPackageName.ToString(),*PackageName);
#endif
						CurrentAssetDependencies.AddUnique(FName(*PackageName));
					}
				}

			}
#endif
		}
		
		if (!bGetDependenciesSuccess)
			return AssetDependencies;
	}
	
	{
		SCOPED_NAMED_EVENT_TEXT("check valid package and ignore rule",FColor::Red);
		
		FCriticalSection	SynchronizationObject;
		ParallelFor(CurrentAssetDependencies.Num(),[&](int32 index)
		{
			auto LongPackageName = CurrentAssetDependencies[index];
			FString LongPackageNameStr = LongPackageName.ToString();
		
			// ignore /Script/ and self
			if(LongPackageName.IsNone() || !IsValidPackageName(LongPackageNameStr) || LongPackageName == InLongPackageName)
			{
				return;
			}
			// check is ignore directories or ingore types
			{
				SCOPED_NAMED_EVENT_TEXT("check ignore directories",FColor::Red);
				if(!IsForceSkipAsset(LongPackageNameStr,IgnoreAssetTypes,IgnoreDirectories,ForceSkipDirectories,TempForceSkipPackageNames,true))
				{
					FScopeLock Lock(&SynchronizationObject);
					AssetDependencies.Add(LongPackageName);
				}
			}
		},true);
	}
	
	if(bRecursively)
	{
		TSet<FName> Dependencies;
#if ASSET_DEPENDENCIES_DEBUG_LOG
		if(bVerboseLog)
		{
			UE_LOG(LogHotPatcher,Display,TEXT("AssetParser %s Dependencies: (%d)"),*InLongPackageName.ToString(),AssetDependencies.Num());
			for(const auto& AssetPackageName:AssetDependencies)
			{
				UE_LOG(LogHotPatcher,Display,TEXT("\t%s"),*AssetPackageName.ToString());
			}
		}
#endif
		for(const auto& AssetPackageName:AssetDependencies)
		{
			if(AssetPackageName.IsNone())
				continue;
			if(TempForceSkipPackageNames.Contains(AssetPackageName.ToString()))
				continue;
				
			if(!InScanedCaches.Contains(AssetPackageName))
			{
				InScanedCaches.Add(AssetPackageName,
					GatherAssetDependicesInfoRecursively(
						InAssetRegistryModule,
						AssetPackageName,
						InAssetDependencyTypes,
						true,
						IgnoreDirectories,
						ForceSkipDirectories,
						TempForceSkipPackageNames,
						IgnoreAssetTypes,
						InScanedCaches
						));
			}
			Dependencies.Append(*InScanedCaches.Find(AssetPackageName));
		}
		AssetDependencies.Append(Dependencies);
	}
	return AssetDependencies;
}
