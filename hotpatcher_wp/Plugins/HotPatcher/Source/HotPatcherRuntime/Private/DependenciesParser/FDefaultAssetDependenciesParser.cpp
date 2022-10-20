#include "DependenciesParser/FDefaultAssetDependenciesParser.h"
#include "FlibAssetManageHelper.h"
#include "HotPatcherLog.h"
#include "HotPatcherRuntime.h"
#include "Async/ParallelFor.h"
#include "Engine/World.h"
#include "Engine/WorldComposition.h"
#include "Resources/Version.h"

void FAssetDependenciesParser::Parse(const FAssetDependencies& InParseConfig)
{
	ParseConfig = InParseConfig;
	
	SCOPED_NAMED_EVENT_TEXT("FAssetDependenciesParser::Parse",FColor::Red);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TSet<FName> AssetPackageNames;
	
	// make /Game/XXX to /Game/XXX/
	TArray<FString> IngnoreFilters;
	for(const auto& IgnoreFilter:ParseConfig.IgnoreFilters)
	{
		IngnoreFilters.AddUnique(UFlibAssetManageHelper::NormalizeContentDir(IgnoreFilter));
	}
	
	{
		SCOPED_NAMED_EVENT_TEXT("Get AssetPackageNames by filter",FColor::Red);
		TArray<FAssetData> AssetDatas;
		UFlibAssetManageHelper::GetAssetsData(ParseConfig.IncludeFilters,AssetDatas,!GForceSingleThread);

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
			Results.Append(FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule,PackageName,ParseConfig.AssetRegistryDependencyTypes,true,IngnoreFilters,TSet<FName>{},ParseConfig.IgnoreAseetTypes,ScanedCaches));
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
			if(UFlibAssetManageHelper::GetAssetsDataByPackageName(LongPackageName,AssetData))
			{
				if(IsIgnoreAsset(AssetData))
				{
					continue;
				}
			}
			Results.Add(FName(*SpecifyAsset.Asset.GetLongPackageName()));
			if(SpecifyAsset.bAnalysisAssetDependencies)
			{
				Results.Append(FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule,FName(*LongPackageName),SpecifyAsset.AssetRegistryDependencyTypes,SpecifyAsset.bAnalysisAssetDependencies,IngnoreFilters,TSet<FName>{},ParseConfig.IgnoreAseetTypes,ScanedCaches));
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
	bool bIsForceSkip = IsForceSkipAsset(LongPackageName,ParseConfig.IgnoreAseetTypes,ParseConfig.IgnoreFilters);
	auto HashPackageFlag = [](uint32 Flags,uint32 CheckFlag)->bool
	{
		return (Flags & CheckFlag) != 0;	
	};
	bool bIsEditorFlag = HashPackageFlag(AssetData.PackageFlags,PKG_EditorOnly);
	
	return bIsForceSkip || bIsEditorFlag;
}

bool FAssetDependenciesParser::IsForceSkipAsset(const FString& LongPackageName,TSet<FName> IgnoreTypes,TArray<FString> IgnoreFilters)
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

	if(bIsIgnore)
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
	if(CurrentAssetData.AssetClass.IsEqual(TEXT("PaperSprite")))
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

TSet<FName> FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(FAssetRegistryModule& InAssetRegistryModule,
                                                                           FName InLongPackageName, const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
                                                                           bool bRecursively, const TArray<FString>& IgnoreDirectories,TSet<FName> IgnorePackageNames,const TSet<FName>& IgnoreAssetTypes,FScanedCachesType& InScanedCaches)
{
	TSet<FName> AssetDependencies;
	SCOPED_NAMED_EVENT_TEXT("GatherAssetDependicesInfoRecursively",FColor::Red);
	IgnorePackageNames.Add(InLongPackageName);

	FAssetData CurrentAssetData;
	UFlibAssetManageHelper::GetAssetsDataByPackageName(InLongPackageName.ToString(),CurrentAssetData);

	bool bGetDependenciesSuccess = false;
	EAssetRegistryDependencyType::Type TotalType = EAssetRegistryDependencyType::None;

	for (const auto& DepType : InAssetDependencyTypes)
	{
		TotalType = (EAssetRegistryDependencyType::Type)((uint8)TotalType | (uint8)UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(DepType));
	}

	TArray<FName> CurrentAssetDependencies;
	
	{
		SCOPED_NAMED_EVENT_TEXT("GetDependencies",FColor::Red);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		bGetDependenciesSuccess = InAssetRegistryModule.Get().GetDependencies(InLongPackageName, CurrentAssetDependencies, TotalType);
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
				if(!IsForceSkipAsset(LongPackageNameStr,IgnoreAssetTypes,IgnoreDirectories))
				{
					FScopeLock Lock(&SynchronizationObject);
					AssetDependencies.Add(LongPackageName);
				}
			}
		},GForceSingleThread);
	}
	
	if(bRecursively)
	{
		TSet<FName> Dependencies;
		for(const auto& AssetPackageName:AssetDependencies)
		{
			if(AssetPackageName.IsNone())
				continue;
			if(IgnorePackageNames.Contains(AssetPackageName))
				continue;
				
			if(!InScanedCaches.Contains(AssetPackageName))
			{
				InScanedCaches.Add(AssetPackageName,GatherAssetDependicesInfoRecursively(InAssetRegistryModule, AssetPackageName, InAssetDependencyTypes,true,IgnoreDirectories,IgnorePackageNames,IgnoreAssetTypes,InScanedCaches));
			}
			Dependencies.Append(*InScanedCaches.Find(AssetPackageName));
		}
		AssetDependencies.Append(Dependencies);
	}
	return AssetDependencies;
}
