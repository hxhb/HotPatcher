#include "DependenciesParser/FDefaultAssetDependenciesParser.h"
#include "FlibAssetManageHelper.h"
#include "HotPatcherLog.h"
#include "Engine/WorldComposition.h"


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
		UFlibAssetManageHelper::GetAssetsData(ParseConfig.IncludeFilters,AssetDatas);

		for(const auto& AssetData:AssetDatas)
		{
			if(!IsIgnoreAsset(AssetData))
			{
				AssetPackageNames.Add(AssetData.PackageName);
				if(ParseConfig.bRedirector && AssetData.IsRedirector())
				{
					if(!AssetData.PackageName.IsNone())
					{
						AssetPackageNames.Append(FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule,AssetData.PackageName,ParseConfig.AssetRegistryDependencyTypes,false,IngnoreFilters,TSet<FName>{},ParseConfig.IgnoreAseetTypes,ScanedCaches));
					}
				}
			}
		}
	}
	
	Results.Append(AssetPackageNames);
	
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
	TSet<FName> ForceSkipPackages;
	for(const auto& SkipContent:ParseConfig.ForceSkipContents)
	{
		for(auto& AssetPackageName:Results)
		{
			FString AssetPackageNameStr = AssetPackageName.ToString();
			if(AssetPackageNameStr.StartsWith(SkipContent))
			{
#if ASSET_DEPENDENCIES_DEBUG_LOG
				UE_LOG(LogHotPatcher,Log,TEXT("Force Skip %s (match %s)"),*AssetPackageNameStr,*SkipContent);
#endif
				ForceSkipPackages.Add(AssetPackageName);
			}
		}
	}

	for(FName SkipPackage:ForceSkipPackages)
	{
		Results.Remove(SkipPackage);
	}
}

bool IsValidPackageName(const FString& LongPackageName)
{
	SCOPED_NAMED_EVENT_TEXT("GatherAssetDependicesInfoRecursively",FColor::Red);
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

	bool bIsIgnore = false;
	FString MatchIgnoreStr;
	if(UFlibAssetManageHelper::UFlibAssetManageHelper::MatchIgnoreTypes(LongPackageName,ParseConfig.IgnoreAseetTypes,MatchIgnoreStr))
	{

		bIsIgnore = true;
	}
	
	if(!bIsIgnore && UFlibAssetManageHelper::MatchIgnoreFilters(LongPackageName,ParseConfig.IgnoreFilters,MatchIgnoreStr))
	{
		bIsIgnore = true;
	}

	if(bIsIgnore)
	{
#if ASSET_DEPENDENCIES_DEBUG_LOG
		UE_LOG(LogHotPatcher,Log,TEXT("Force Skip %s (match ignore rule %s)"),*AssetData.GetFullName(),*MatchIgnoreStr);
#endif
	}
	return bIsIgnore;
}

TSet<FName> FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(FAssetRegistryModule& InAssetRegistryModule,
                                                                           FName InLongPackageName, const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
                                                                           bool bRecursively, const TArray<FString>& IgnoreDirectories,TSet<FName> IgnorePackageNames,const TSet<FName>& IgnoreAssetTypes,FScanedCachesType& ScanedCaches)
{
	TSet<FName> AssetDependencies;
	SCOPED_NAMED_EVENT_TEXT("GatherAssetDependicesInfoRecursively",FColor::Red);
	IgnorePackageNames.Add(InLongPackageName);

	FAssetData CurrentAssetData;
	UFlibAssetManageHelper::GetAssetsDataByPackageName(InLongPackageName.ToString(),CurrentAssetData);
	
	// TArray<EAssetRegistryDependencyType::Type> AssetTypes;
	
	bool bGetDependenciesSuccess = false;
	EAssetRegistryDependencyType::Type TotalType = EAssetRegistryDependencyType::None;

	for (const auto& DepType : InAssetDependencyTypes)
	{
		TotalType = (EAssetRegistryDependencyType::Type)((uint8)TotalType | (uint8)UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(DepType));
	}

	TArray<FName> CurrentAssetDependencies;
	
	// AssetTypes.AddUnique(UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(DepType));
	{
		SCOPED_NAMED_EVENT_TEXT("GetDependencies",FColor::Red);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		bGetDependenciesSuccess = InAssetRegistryModule.Get().GetDependencies(InLongPackageName, CurrentAssetDependencies, TotalType);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		// collect world composition tile packages to cook 
		if(CurrentAssetData.GetClass() == UWorld::StaticClass())
		{
			UWorld* World = UWorld::FindWorldInPackage(CurrentAssetData.GetAsset()->GetPackage());
			if (World)
			{
				if(World->WorldComposition)
				{
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
		}
		
		if (!bGetDependenciesSuccess)
			return AssetDependencies;
	}
	
	{
		SCOPED_NAMED_EVENT_TEXT("check valid package and ignore rule",FColor::Red);
		for (auto &LongPackageName : CurrentAssetDependencies)
		{
			FString LongPackageNameStr = LongPackageName.ToString();
		
			// ignore /Script/ and self
			if(LongPackageName.IsNone() || !IsValidPackageName(LongPackageNameStr) || LongPackageName == InLongPackageName)
			{
				continue;
			}
			// check is ignore directories or ingore types
			{
				SCOPED_NAMED_EVENT_TEXT("check ignore directories",FColor::Red);
				bool bIsIgnore = false;
				FString MatchIgnoreStr;
				
				if(!bIsIgnore && UFlibAssetManageHelper::MatchIgnoreTypes(LongPackageNameStr,IgnoreAssetTypes,MatchIgnoreStr))
				{
					bIsIgnore = true;
				}
				
				if(!bIsIgnore && UFlibAssetManageHelper::MatchIgnoreFilters(LongPackageNameStr,IgnoreDirectories,MatchIgnoreStr))
				{
					bIsIgnore = true;
				}

				if(bIsIgnore)
				{
#if ASSET_DEPENDENCIES_DEBUG_LOG
					UE_LOG(LogHotPatcher,Log,TEXT("Force Skip %s (match ignore rule %s)"),*LongPackageNameStr,*MatchIgnoreStr);
#endif
					continue;
				}
				else
				{
					AssetDependencies.Add(LongPackageName);
				}
			}
		}
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
				
			if(!ScanedCaches.Contains(AssetPackageName))
			{
				ScanedCaches.Add(AssetPackageName,GatherAssetDependicesInfoRecursively(InAssetRegistryModule, AssetPackageName, InAssetDependencyTypes,true,IgnoreDirectories,IgnorePackageNames,IgnoreAssetTypes,ScanedCaches));
			}
			Dependencies.Append(*ScanedCaches.Find(AssetPackageName));
		}
		AssetDependencies.Append(Dependencies);
	}
	return AssetDependencies;
}
