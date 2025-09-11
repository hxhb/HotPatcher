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
	SCOPED_NAMED_EVENT_TEXT("GatherAssetDependicesInfoRecursively", FColor::Red);
	TSet<FString> TempForceSkipPackageNames = ForceSkipPackageNames;
	TempForceSkipPackageNames.Add(InLongPackageName.ToString());

	FAssetData CurrentAssetData;
	UFlibAssetManageHelper::GetAssetsDataByPackageName(InLongPackageName.ToString(), CurrentAssetData);

	// --- NEW: 把自定义的 Ex 数组转成 5.6 的 Options ---
	auto MakeOptionsFromExArray = [](const TArray<EAssetRegistryDependencyTypeEx>& Types)
		{
			FAssetRegistryDependencyOptions Opt;
			Opt.bIncludeSoftPackageReferences = false;
			Opt.bIncludeHardPackageReferences = false;
			Opt.bIncludeSearchableNames = false;
			Opt.bIncludeSoftManagementReferences = false;
			Opt.bIncludeHardManagementReferences = false;

			auto Enable = [&Opt](EAssetRegistryDependencyTypeEx T)
				{
					switch (T)
					{
					case EAssetRegistryDependencyTypeEx::Soft:
						Opt.bIncludeSoftPackageReferences = true; break;
					case EAssetRegistryDependencyTypeEx::Hard:
						Opt.bIncludeHardPackageReferences = true; break;
					case EAssetRegistryDependencyTypeEx::SearchableName:
						Opt.bIncludeSearchableNames = true; break;
					case EAssetRegistryDependencyTypeEx::SoftManage:
						Opt.bIncludeSoftManagementReferences = true; break;
					case EAssetRegistryDependencyTypeEx::HardManage:
						Opt.bIncludeHardManagementReferences = true; break;
					case EAssetRegistryDependencyTypeEx::Packages:
						Opt.bIncludeSoftPackageReferences = true;
						Opt.bIncludeHardPackageReferences = true; break;
					case EAssetRegistryDependencyTypeEx::Manage:
						Opt.bIncludeSoftManagementReferences = true;
						Opt.bIncludeHardManagementReferences = true; break;
					case EAssetRegistryDependencyTypeEx::All:
						Opt.bIncludeSoftPackageReferences = true;
						Opt.bIncludeHardPackageReferences = true;
						Opt.bIncludeSearchableNames = true;
						Opt.bIncludeSoftManagementReferences = true;
						Opt.bIncludeHardManagementReferences = true; break;
					case EAssetRegistryDependencyTypeEx::None:
					default: break;
					}
				};

			for (EAssetRegistryDependencyTypeEx T : Types) Enable(T);
			return Opt;
		};

	const FAssetRegistryDependencyOptions Options = MakeOptionsFromExArray(InAssetDependencyTypes);

	TArray<FName> CurrentAssetDependencies;

	{
		SCOPED_NAMED_EVENT_TEXT("GetDependencies", FColor::Red);

		// --- NEW: 用 K2_ 版本并传 Options ---
		const bool bGetDependenciesSuccess = InAssetRegistryModule.Get().K2_GetDependencies(
			InLongPackageName, Options, CurrentAssetDependencies);

		if (!bGetDependenciesSuccess)
		{
			return AssetDependencies;
		}

		// 你的 PaperSprite / WorldComposition 过滤与补充逻辑保持不变
		for (const auto& SkipForDependencies : ParserSkipAssetByDependencies(CurrentAssetData, CurrentAssetDependencies))
		{
			CurrentAssetDependencies.Remove(SkipForDependencies);
		}

#if WITH_EDITOR
		if (ParseConfig.bSupportWorldComposition && CurrentAssetData.GetClass() == UWorld::StaticClass())
		{
			FString DisplayStr = FString::Printf(TEXT("LoadWorld %s"), *CurrentAssetData.GetFullName());
			FScopedNamedEvent CacheClassEvent(FColor::Red, *DisplayStr);
			UWorld* World = UWorld::FindWorldInPackage(CurrentAssetData.GetAsset()->GetOutermost());
			if (World && World->WorldComposition)
			{
				SCOPED_NAMED_EVENT_TEXT("Collect WorldComposition Tiles", FColor::Red);
				TArray<FString> PackageNames;
				World->WorldComposition->CollectTilesToCook(PackageNames);
				for (const auto& PackageName : PackageNames)
				{
#if ASSET_DEPENDENCIES_DEBUG_LOG
					UE_LOG(LogHotPatcher, Log, TEXT("Collecting WorldComposition Tile Package %s for %s"), *InLongPackageName.ToString(), *PackageName);
#endif
					CurrentAssetDependencies.AddUnique(FName(*PackageName));
				}
			}
		}
#endif
	}

	// 下面并发过滤与递归聚合逻辑保持不变
	{
		SCOPED_NAMED_EVENT_TEXT("check valid package and ignore rule", FColor::Red);
		FCriticalSection SynchronizationObject;

		ParallelFor(CurrentAssetDependencies.Num(), [&](int32 index)
			{
				FName LongPackageName = CurrentAssetDependencies[index];
				const FString LongPackageNameStr = LongPackageName.ToString();

				if (LongPackageName.IsNone() || !IsValidPackageName(LongPackageNameStr) || LongPackageName == InLongPackageName)
				{
					return;
				}
				if (!IsForceSkipAsset(LongPackageNameStr, IgnoreAssetTypes, IgnoreDirectories, ForceSkipDirectories, TempForceSkipPackageNames, true))
				{
					FScopeLock Lock(&SynchronizationObject);
					AssetDependencies.Add(LongPackageName);
				}
			}, true);
	}

	if (bRecursively)
	{
		TSet<FName> Dependencies;
#if ASSET_DEPENDENCIES_DEBUG_LOG
		if (bVerboseLog)
		{
			UE_LOG(LogHotPatcher, Display, TEXT("AssetParser %s Dependencies: (%d)"), *InLongPackageName.ToString(), AssetDependencies.Num());
			for (const auto& AssetPackageName : AssetDependencies)
			{
				UE_LOG(LogHotPatcher, Display, TEXT("\t%s"), *AssetPackageName.ToString());
			}
		}
#endif
		for (const auto& AssetPackageName : AssetDependencies)
		{
			if (AssetPackageName.IsNone()) continue;
			if (TempForceSkipPackageNames.Contains(AssetPackageName.ToString())) continue;

			if (!InScanedCaches.Contains(AssetPackageName))
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
						InScanedCaches));
			}
			Dependencies.Append(*InScanedCaches.Find(AssetPackageName));
		}
		AssetDependencies.Append(Dependencies);
	}

	return AssetDependencies;
}

