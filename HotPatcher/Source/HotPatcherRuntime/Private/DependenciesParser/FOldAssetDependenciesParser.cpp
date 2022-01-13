#include "DependenciesParser/FOldAssetDependenciesParser.h"
#include "HotPatcherLog.h"

bool GScanCacheOptimize = false;
namespace NSOldAssetDependenciesParser
{
		/*
	 @Param InLongPackageName: e.g /Game/TEST/BP_Actor don't pass /Game/TEST/BP_Actor.BP_Actor
	 @Param OutDependices: Output Asset Dependencies
	*/
		static void GetAssetDependencies(
			const FString& InLongPackageName,
			const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
			FAssetDependenciesInfo& OutDependices,
			TMap<FString, FAssetDependenciesInfo>& ScanedCaches
		);
		static bool GetAssetDependency(
			const FString& InLongPackageName,
			const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
			TArray<FAssetDetail>& OutRefAsset,
			TMap<FString, FAssetDependenciesInfo>& ScandCaches,
			bool bRecursively = true
		);
		static void GetAssetDependenciesForAssetDetail(
			const FAssetDetail& InAssetDetail,
			const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
			FAssetDependenciesInfo& OutDependices,
			TMap<FString, FAssetDependenciesInfo>& ScandCaches
		);
		static void GetAssetListDependenciesForAssetDetail(
			const TArray<FAssetDetail>& InAssetsDetailList, 
			const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
			FAssetDependenciesInfo& OutDependices,
			TMap<FString, FAssetDependenciesInfo>& ScandCaches
		);
	
	// recursive scan assets
	static void GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		const FString& InTargetLongPackageName,
		const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
		FAssetDependenciesInfo& OutDependencies,
		TMap<FString,FAssetDependenciesInfo>& ScanedCaches,
		bool bRecursively=true,
		TSet<FString> IgnoreAssetsPackageNames=TSet<FString>{}
	);
	
	/*
	 * FilterPackageName format is /Game or /Game/TEST
	 */
	static bool GetAssetsList(	const TArray<FString>& InFilterPaths,
			const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
			TArray<FAssetDetail>& OutAssetList,
			TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
			bool bReTargetRedirector=true
		);
}
void FOldAssetDependenciesParser::Parse(const FAssetDependencies& ParseConfig)
{
		TArray<FAssetDetail> FilterAssetList;
		{
			TArray<FAssetDetail> AllNeedPakRefAssets;
			{
				SCOPED_NAMED_EVENT_TCHAR(TEXT("parser asset by filter"),FColor::Red);
				TArray<FAssetDetail> AllAssets;
				NSOldAssetDependenciesParser::GetAssetsList(ParseConfig.IncludeFilters, ParseConfig.AssetRegistryDependencyTypes, AllAssets,ScanedCaches);
				if (ParseConfig.IncludeHasRefAssetsOnly)
				{
					TArray<FAssetDetail> AllDontHasRefAssets;
					UFlibAssetManageHelper::FilterNoRefAssetsWithIgnoreFilter(AllAssets, ParseConfig.IgnoreFilters, AllNeedPakRefAssets, AllDontHasRefAssets);
				}
				else
				{
					AllNeedPakRefAssets = AllAssets;
				}
			}
			// 剔除ignore filter中指定的资源
			if (ParseConfig.IgnoreFilters.Num() > 0)
			{
				SCOPED_NAMED_EVENT_TCHAR(TEXT("remove asset by IgnoreFilter"),FColor::Red);
				for (const auto& AssetDetail : AllNeedPakRefAssets)
				{
					bool bIsIgnore = false;
					for (const auto& IgnoreFilter : ParseConfig.IgnoreFilters)
					{
						if (AssetDetail.PackagePath.ToString().StartsWith(IgnoreFilter))
						{
							bIsIgnore = true;
							break;
						}
					}
					if (!bIsIgnore)
					{
						FilterAssetList.Add(AssetDetail);
					}
				}
			}
			else
			{
				FilterAssetList = AllNeedPakRefAssets;
			}
		
		}
		
		auto AnalysisAssetDependency = [this](const TArray<FAssetDetail>& InAssetDetail, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes, bool bInAnalysisDepend)->FAssetDependenciesInfo
		{
			SCOPED_NAMED_EVENT_TCHAR(TEXT("ExportReleaseVersionInfo AnalysisAssetDependency"),FColor::Red);
			FAssetDependenciesInfo RetAssetDepend;
			if (InAssetDetail.Num())
			{
				UFlibAssetManageHelper::CombineAssetsDetailAsFAssetDepenInfo(InAssetDetail, RetAssetDepend);
		
				if (bInAnalysisDepend)
				{
					FAssetDependenciesInfo AssetDependencies;
					NSOldAssetDependenciesParser::GetAssetListDependenciesForAssetDetail(InAssetDetail,AssetRegistryDependencyTypes, AssetDependencies,ScanedCaches);
		
					RetAssetDepend = UFlibAssetManageHelper::CombineAssetDependencies(RetAssetDepend, AssetDependencies);
				}
			}
			return RetAssetDepend;
		};
		
		// 分析过滤器中指定的资源依赖
		FAssetDependenciesInfo FilterAssetDependencies = AnalysisAssetDependency(FilterAssetList, ParseConfig.AssetRegistryDependencyTypes, ParseConfig.AnalysicFilterDependencies);
		
		// Specify Assets
		FAssetDependenciesInfo SpecifyAssetDependencies;
		{
			SCOPED_NAMED_EVENT_TCHAR(TEXT("ExportReleaseVersionInfo parser Specify Assets"),FColor::Red);
			for (const auto& SpecifyAsset : ParseConfig.InIncludeSpecifyAsset)
			{
				FString AssetLongPackageName = SpecifyAsset.Asset.GetLongPackageName();
				FAssetDetail AssetDetail;
				if (UFlibAssetManageHelper::GetSpecifyAssetDetail(AssetLongPackageName, AssetDetail))
				{
					FAssetDependenciesInfo CurrentAssetDependencies = AnalysisAssetDependency(TArray<FAssetDetail>{AssetDetail}, SpecifyAsset.AssetRegistryDependencyTypes, SpecifyAsset.bAnalysisAssetDependencies);
					SpecifyAssetDependencies = UFlibAssetManageHelper::CombineAssetDependencies(SpecifyAssetDependencies, CurrentAssetDependencies);
				}
			}
		}
		
		Results = UFlibAssetManageHelper::CombineAssetDependencies(FilterAssetDependencies, SpecifyAssetDependencies);
	}




void NSOldAssetDependenciesParser::GetAssetDependencies(
	const FString& InLongPackageName,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	FAssetDependenciesInfo& OutDependices,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("NSOldAssetDependenciesParser::GetAssetDependencies"),FColor::Red);
	if (InLongPackageName.IsEmpty())
		return;

	FSoftObjectPath AssetRef = FSoftObjectPath(InLongPackageName);
	if (!AssetRef.IsValid())
		return;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	if (FPackageName::DoesPackageExist(InLongPackageName))
	{
		{
			TArray<FAssetData> AssetDataList;
			bool bResault = AssetRegistryModule.Get().GetAssetsByPackageName(FName(*InLongPackageName), AssetDataList);
			if (!bResault || !AssetDataList.Num())
			{
				UE_LOG(LogHotPatcher, Error, TEXT("Faild to Parser AssetData of %s, please check."), *InLongPackageName);
				return;
			}
			if (AssetDataList.Num() > 1)
			{
				for(const auto& Asset:AssetDataList)
				{
					UE_LOG(LogHotPatcher,Log,TEXT("AssetData ObjectPath %s"),*Asset.ObjectPath.ToString())
					UE_LOG(LogHotPatcher,Log,TEXT("AssetData AssetName %s"),*Asset.AssetName.ToString())
				}
				UE_LOG(LogHotPatcher, Warning, TEXT("Got mulitple AssetData of %s,please check."), *InLongPackageName);
			}
		}
		NSOldAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule, InLongPackageName, AssetRegistryDependencyTypes, OutDependices,ScanedCaches);
	}
}

bool NSOldAssetDependenciesParser::GetAssetDependency(
	const FString& InLongPackageName,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	TArray<FAssetDetail>& OutRefAsset,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
	bool bRecursively
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("NSOldAssetDependenciesParser::GetAssetDependency"),FColor::Red);
	bool bStatus = false;
	if (FPackageName::DoesPackageExist(InLongPackageName))
	{
		OutRefAsset.Empty();
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		FAssetDependenciesInfo AssetDep;
		NSOldAssetDependenciesParser::GatherAssetDependicesInfoRecursively(AssetRegistryModule, InLongPackageName, AssetRegistryDependencyTypes, AssetDep,ScanedCaches,bRecursively);
		OutRefAsset = AssetDep.GetAssetDetails();
		bStatus = true;
	}
	return bStatus;
}

void NSOldAssetDependenciesParser::GetAssetDependenciesForAssetDetail(
	const FAssetDetail& InAssetDetail,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	FAssetDependenciesInfo& OutDependices,
	TMap<FString, FAssetDependenciesInfo>& ScandCaches
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetDependenciesForAssetDetail"),FColor::Red);
	FSoftObjectPath AssetObjectSoftRef{ InAssetDetail.PackagePath.ToString() };

	NSOldAssetDependenciesParser::GetAssetDependencies(AssetObjectSoftRef.GetLongPackageName(), AssetRegistryDependencyTypes, OutDependices,ScandCaches);
}

void NSOldAssetDependenciesParser::GetAssetListDependenciesForAssetDetail(
			const TArray<FAssetDetail>& InAssetsDetailList, 
			const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
			FAssetDependenciesInfo& OutDependices,
			TMap<FString, FAssetDependenciesInfo>& ScandCaches
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("NSOldAssetDependenciesParser::GetAssetListDependenciesForAssetDetail"),FColor::Red);
	FAssetDependenciesInfo result;
	for (const auto& AssetDetail : InAssetsDetailList)
	{
		FString AssetPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(AssetDetail.PackagePath.ToString());
		FAssetDependenciesInfo CurrentAsserInfo;

		if(GScanCacheOptimize && ScandCaches.Find(AssetPackageName))
		{
			CurrentAsserInfo = *ScandCaches.Find(AssetPackageName);
		}
		else
		{
			NSOldAssetDependenciesParser::GetAssetDependenciesForAssetDetail(AssetDetail, AssetRegistryDependencyTypes,CurrentAsserInfo,ScandCaches);
			if(GScanCacheOptimize && !ScandCaches.Find(AssetPackageName))
			{
				ScandCaches.Add(AssetPackageName,CurrentAsserInfo);
			}
		}
		
		result = UFlibAssetManageHelper::CombineAssetDependencies(result, CurrentAsserInfo);
	}
	OutDependices = result;
}


void NSOldAssetDependenciesParser::GatherAssetDependicesInfoRecursively(
	FAssetRegistryModule& InAssetRegistryModule,
	const FString& InTargetLongPackageName,
	const TArray<EAssetRegistryDependencyTypeEx>& InAssetDependencyTypes,
	FAssetDependenciesInfo& OutDependencies,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
	bool bRecursively,
	TSet<FString> IgnoreAssetsPackageNames
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("NSOldAssetDependenciesParser::GatherAssetDependicesInfoRecursively"),FColor::Red);
	IgnoreAssetsPackageNames.Add(InTargetLongPackageName);
	TArray<FName> local_Dependencies;
	// TArray<EAssetRegistryDependencyType::Type> AssetTypes;
	
	bool bGetDependenciesSuccess = false;
	EAssetRegistryDependencyType::Type TotalType = EAssetRegistryDependencyType::None;

	for (const auto& DepType : InAssetDependencyTypes)
	{
		TotalType = (EAssetRegistryDependencyType::Type)((uint8)TotalType | (uint8)UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(DepType));
	}

	// AssetTypes.AddUnique(UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(DepType));
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	bGetDependenciesSuccess = InAssetRegistryModule.Get().GetDependencies(FName(*InTargetLongPackageName), local_Dependencies, TotalType);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	if (!bGetDependenciesSuccess)
		return;

	TMap<FString,FAssetDetail> AssetDependenciesDetailMap;
		
	for (auto &DependItem : local_Dependencies)
	{
		FString LongDependentPackageName = DependItem.ToString();

		// ignore /Script/WeatherSystem and self
		if(LongDependentPackageName.StartsWith(TEXT("/Script/")) || LongDependentPackageName.Equals(InTargetLongPackageName))
			continue;

		FAssetDetail AssetDetail = UFlibAssetManageHelper::GetAssetDetailByPackageName(LongDependentPackageName);
		if(AssetDetail.IsValid())
		{
			AssetDependenciesDetailMap.Add(LongDependentPackageName,AssetDetail);
		}
	}

	for(const auto& AssetDetail:AssetDependenciesDetailMap)
	{
		if(AssetDetail.Value.PackagePath.IsNone())
			continue;
		FString BelongModuleName = UFlibAssetManageHelper::GetAssetBelongModuleName(AssetDetail.Key);
		FAssetDependenciesDetail* ModuleCategory;
		if (OutDependencies.AssetsDependenciesMap.Contains(BelongModuleName))
		{
			// UE_LOG(LogHotPatcher, Log, TEXT("Belong Module is %s,Asset is %s"), *BelongModuleName, *LongDependentPackageName);
			ModuleCategory = OutDependencies.AssetsDependenciesMap.Find(BelongModuleName);
		}
		else
		{
			// UE_LOG(LogHotPatcher, Log, TEXT("New Belong Module is %s,Asset is %s"), *BelongModuleName,*LongDependentPackageName);
			FAssetDependenciesDetail& NewModuleCategory = OutDependencies.AssetsDependenciesMap.Add(BelongModuleName, FAssetDependenciesDetail{});
			NewModuleCategory.ModuleCategory = BelongModuleName;
			ModuleCategory = OutDependencies.AssetsDependenciesMap.Find(BelongModuleName);
		}
		
		ModuleCategory->AssetDependencyDetails.Add(AssetDetail.Key, AssetDetail.Value);
		
		if (bRecursively && !IgnoreAssetsPackageNames.Contains(AssetDetail.Key))
		{
			if(!GScanCacheOptimize)
			{
				NSOldAssetDependenciesParser::GatherAssetDependicesInfoRecursively(InAssetRegistryModule, AssetDetail.Key, InAssetDependencyTypes,OutDependencies, ScanedCaches,true,IgnoreAssetsPackageNames);
			}
			else
			{
				FAssetDependenciesInfo CurrentDependencies;
				// search cached
				if(!ScanedCaches.Find(AssetDetail.Key))
				{
					NSOldAssetDependenciesParser::GatherAssetDependicesInfoRecursively(InAssetRegistryModule, AssetDetail.Key, InAssetDependencyTypes,CurrentDependencies, ScanedCaches,true,IgnoreAssetsPackageNames);
					// Add to ScanedCaches
					ScanedCaches.Add(AssetDetail.Key,CurrentDependencies);
				}
				else
				{
					CurrentDependencies = *ScanedCaches.Find(AssetDetail.Key);
				}
				
				OutDependencies = UFlibAssetManageHelper::CombineAssetDependencies(OutDependencies,CurrentDependencies);
			}
		}
	}
}

bool NSOldAssetDependenciesParser::GetAssetsList(
	const TArray<FString>& InFilterPaths,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	TArray<FAssetDetail>& OutAssetList,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
	bool bReTargetRedirector
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetsList"),FColor::Red);
	TArray<FAssetData> AllAssetData;
	if (UFlibAssetManageHelper::GetAssetsData(InFilterPaths, AllAssetData,true))
	{
		for (const auto& AssetDataIndex : AllAssetData)
		{
			if (!AssetDataIndex.IsValid())
				continue;
			FAssetDetail AssetDetail;
			if (bReTargetRedirector && AssetDataIndex.IsRedirector())
			{
				TArray<FAssetDetail> TargetAssets;
				FString RedirectorLongPackageName = UFlibAssetManageHelper::LongPackageNameToPackagePath(AssetDataIndex.PackageName.ToString());
				{
					if(!RedirectorLongPackageName.IsEmpty())
					{
						NSOldAssetDependenciesParser::GetAssetDependency(RedirectorLongPackageName, AssetRegistryDependencyTypes, TargetAssets, ScanedCaches,false);
						if (!!TargetAssets.Num())
						{
							AssetDetail = TargetAssets[0];
						}
					}
				}
			}
			else
			{
				UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(AssetDataIndex, AssetDetail);
			}
			if (!AssetDetail.PackagePath.IsNone())
			{
				OutAssetList.Add(AssetDetail);
			}
		}
		return true;
	}
	return false;
}