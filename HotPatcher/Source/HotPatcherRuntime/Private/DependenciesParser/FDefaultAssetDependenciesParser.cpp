#include "DependenciesParser/FDefaultAssetDependenciesParser.h"
#include "FlibAssetManageHelper.h"
#include "HotPatcherLog.h"


void FAssetDependenciesParser::Parse(const FAssetDependencies& ParseConfig)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("FAssetDependenciesParser::Parse"),FColor::Red);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TSet<FName> AssetPackageNames;
	
	// make /Game/XXX to /Game/XXX/
	TArray<FString> IngnoreFilters;
	for(const auto& IgnoreFilter:ParseConfig.IgnoreFilters)
	{
		IngnoreFilters.AddUnique(UFlibAssetManageHelper::NormalizeContentDir(IgnoreFilter));
	}
	
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("Get AssetPackageNames by filter"),FColor::Red);
		TArray<FAssetData> AssetDatas;
		UFlibAssetManageHelper::GetAssetsData(ParseConfig.IncludeFilters,AssetDatas);

		for(const auto& AssetData:AssetDatas)
		{
			bool bIsIgnore = false;
			for(const auto& IgnoreFilter:ParseConfig.IgnoreFilters)
			{
				if(AssetData.PackageName.ToString().StartsWith(IgnoreFilter))
				{
					bIsIgnore = true;
					break;
				}
			}
			if(!bIsIgnore)
			{
				AssetPackageNames.Add(AssetData.PackageName);
				if(ParseConfig.bRedirector && AssetData.IsRedirector())
				{
					if(!AssetData.PackageName.IsNone())
					{
						AssetPackageNames.Append(FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule,AssetData.PackageName,ParseConfig.AssetRegistryDependencyTypes,false,IngnoreFilters,TSet<FName>{},ParseConfig.IgnoreDependenciesAseetTypes,ScanedCaches));
					}
				}
			}
		}
	}
	
	Results.Append(AssetPackageNames);
	
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("Get dependencies for filters"),FColor::Red);
		for(FName PackageName:AssetPackageNames)
		{
			Results.Append(FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule,PackageName,ParseConfig.AssetRegistryDependencyTypes,true,IngnoreFilters,TSet<FName>{},ParseConfig.IgnoreDependenciesAseetTypes,ScanedCaches));
		}
	}
	
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("Get dependencies for SpecifyAsset"),FColor::Red);
		for(const auto& SpecifyAsset:ParseConfig.InIncludeSpecifyAsset)
		{
			if(!SpecifyAsset.Asset.IsValid())
			{
				continue;
			}
			Results.Add(FName(SpecifyAsset.Asset.GetLongPackageName()));
			if(SpecifyAsset.bAnalysisAssetDependencies)
			{
				Results.Append(FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule,FName(SpecifyAsset.Asset.GetLongPackageName()),SpecifyAsset.AssetRegistryDependencyTypes,SpecifyAsset.bAnalysisAssetDependencies,IngnoreFilters,TSet<FName>{},ParseConfig.IgnoreDependenciesAseetTypes,ScanedCaches));
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
				UE_LOG(LogHotPatcher,Display,TEXT("Force Skip %s (match %s)"),*AssetPackageNameStr,*SkipContent);
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
	SCOPED_NAMED_EVENT_TCHAR(TEXT("GatherAssetDependicesInfoRecursively"),FColor::Red);
	bool bStatus = false;
	if (!LongPackageName.IsEmpty() && !FPackageName::IsScriptPackage(LongPackageName) && !FPackageName::IsMemoryPackage(LongPackageName))
	{
		bStatus = true;
	}
	return bStatus;
}

TSet<FName> FAssetDependenciesParser::GatherAssetDependicesInfoRecursively(FAssetRegistryModule& InAssetRegistryModule,
	FName InLongPackageName, const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
	bool bRecursively, const TArray<FString>& IgnoreDirectories,TSet<FName> IgnorePackageNames,const TSet<FName>& IgnoreAssetTypes,FScanedCachesType& ScanedCaches)
{
	TSet<FName> AssetDependencies;
	SCOPED_NAMED_EVENT_TCHAR(TEXT("GatherAssetDependicesInfoRecursively"),FColor::Red);
	IgnorePackageNames.Add(InLongPackageName);
	
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
		SCOPED_NAMED_EVENT_TCHAR(TEXT("GetDependencies"),FColor::Red);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		bGetDependenciesSuccess = InAssetRegistryModule.Get().GetDependencies(InLongPackageName, CurrentAssetDependencies, TotalType);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		if (!bGetDependenciesSuccess)
			return AssetDependencies;
	}
	
	{
		SCOPED_NAMED_EVENT_TCHAR(TEXT("check valid package and ignore rule"),FColor::Red);
		for (auto &LongPackageName : CurrentAssetDependencies)
		{
			FString LongPackageNameStr = LongPackageName.ToString();
		
			// ignore /Script/ and self
			if(LongPackageName.IsNone() || !IsValidPackageName(LongPackageNameStr) || LongPackageName == InLongPackageName)
			{
				continue;
			}

			// check is ignore directories
			{
				SCOPED_NAMED_EVENT_TCHAR(TEXT("check ignore directories"),FColor::Red);
				bool bIgnoreDirectories = false;
				for(const auto& IgnoreDirectory:IgnoreDirectories)
				{
					if(LongPackageNameStr.StartsWith(IgnoreDirectory))
					{
						bIgnoreDirectories = true;
						break;
					}
				}
				if(bIgnoreDirectories)
				{
#if ASSET_DEPENDENCIES_DEBUG_LOG
					UE_LOG(LogHotPatcher,Display,TEXT("Ignore %s (match ignore dirctories)"),*LongPackageNameStr);
#endif
					continue;;
				}
			}

			// check is ignore types
			{
				SCOPED_NAMED_EVENT_TCHAR(TEXT("check ignore types"),FColor::Red);
				bool bIgnoreType = false;
				FString AssetTypeName;
				{
					FAssetData AssetData;
					if(UFlibAssetManageHelper::GetAssetsDataByPackageName(LongPackageNameStr,AssetData))
					{
						SCOPED_NAMED_EVENT_TCHAR(TEXT("is ignore types"),FColor::Red);
						AssetTypeName = AssetData.AssetClass.ToString();
						bIgnoreType = IgnoreAssetTypes.Contains(AssetData.AssetClass);
					}
				}
				if(!bIgnoreType)
				{
					AssetDependencies.Add(LongPackageName);
				}
				else
				{
#if ASSET_DEPENDENCIES_DEBUG_LOG
					SCOPED_NAMED_EVENT_TCHAR(TEXT("display ignore log"),FColor::Red);
					UE_LOG(LogHotPatcher,Display,TEXT("Ignore %s by %s dependencies (%s)"),*LongPackageName.ToString(),*InLongPackageName.ToString(),*AssetTypeName);
#endif
					continue;
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
