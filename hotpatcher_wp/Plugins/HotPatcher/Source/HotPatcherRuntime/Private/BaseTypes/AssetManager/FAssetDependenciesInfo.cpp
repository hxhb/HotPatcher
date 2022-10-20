#include "AssetManager/FAssetDependenciesInfo.h"
#include "FlibAssetManageHelper.h"
#include "HotPatcherRuntime.h"
#include "Async/ParallelFor.h"


void FAssetDependenciesInfo::AddAssetsDetail(const FAssetDetail& AssetDetail)
{
	if(!AssetDetail.IsValid())
		return;
	FString CurrAssetModuleName = UFlibAssetManageHelper::GetAssetBelongModuleName(AssetDetail.PackagePath.ToString());
	FSoftObjectPath CurrAssetObjectPath(AssetDetail.PackagePath.ToString());
	FString CurrAssetLongPackageName = CurrAssetObjectPath.GetLongPackageName();
	if (!AssetsDependenciesMap.Contains(CurrAssetModuleName))
	{
		FAssetDependenciesDetail AssetDependenciesDetail{ CurrAssetModuleName,TMap<FString,FAssetDetail>{ {CurrAssetLongPackageName,AssetDetail} } };
		AssetsDependenciesMap.Add(CurrAssetModuleName, AssetDependenciesDetail);
	}
	else
	{
		FAssetDependenciesDetail& CurrentCategory = *AssetsDependenciesMap.Find(CurrAssetModuleName);
		
		if (!CurrentCategory.AssetDependencyDetails.Contains(CurrAssetLongPackageName))
		{
			CurrentCategory.AssetDependencyDetails.Add(CurrAssetLongPackageName,AssetDetail);
		}
	}
}

bool FAssetDependenciesInfo::HasAsset(const FString& InAssetPackageName)const
{
	SCOPED_NAMED_EVENT_TEXT("IsHasAsset",FColor::Red);
	bool bHas = false;
	FString BelongModuleName = UFlibAssetManageHelper::GetAssetBelongModuleName(InAssetPackageName);
	if (AssetsDependenciesMap.Contains(BelongModuleName))
	{
		bHas = AssetsDependenciesMap.Find(BelongModuleName)->AssetDependencyDetails.Contains(InAssetPackageName);
	}
	return bHas;
}

TArray<FAssetDetail> FAssetDependenciesInfo::GetAssetDetails()const
{
	SCOPED_NAMED_EVENT_TEXT("FAssetDependenciesInfo::GetAssetDetails",FColor::Red);
	TArray<FAssetDetail> OutAssetDetails;
	
	OutAssetDetails.Empty();
	TArray<FString> Keys;
	AssetsDependenciesMap.GetKeys(Keys);

	FCriticalSection	SynchronizationObject;
	ParallelFor(Keys.Num(),[&](int32 index)
	{
		const TMap<FString, FAssetDetail>& ModuleAssetDetails = AssetsDependenciesMap.Find(Keys[index])->AssetDependencyDetails;

		TArray<FString> ModuleAssetKeys;
		ModuleAssetDetails.GetKeys(ModuleAssetKeys);

		for (const auto& ModuleAssetKey : ModuleAssetKeys)
		{
			FScopeLock Lock(&SynchronizationObject);
			OutAssetDetails.Add(*ModuleAssetDetails.Find(ModuleAssetKey));
		}
	},GForceSingleThread);
	
	return OutAssetDetails;
}

bool FAssetDependenciesInfo::GetAssetDetailByPackageName(const FString& InAssetPackageName,FAssetDetail& OutDetail) const
{
	bool bHas = HasAsset(InAssetPackageName);
	if(bHas)
	{
		FString BelongModuleName = UFlibAssetManageHelper::GetAssetBelongModuleName(InAssetPackageName);
		OutDetail = *AssetsDependenciesMap.Find(BelongModuleName)->AssetDependencyDetails.Find(InAssetPackageName);
	}
	return bHas;
}

TArray<FString> FAssetDependenciesInfo::GetAssetLongPackageNames()const
{
	SCOPED_NAMED_EVENT_TEXT("FAssetDependenciesInfo::GetAssetLongPackageNames",FColor::Red);
	TArray<FString> OutAssetLongPackageName;
	const TArray<FAssetDetail>& OutAssetDetails = GetAssetDetails();

	for (const auto& Asset : OutAssetDetails)
	{
		FString LongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(Asset.PackagePath.ToString());
		{
			OutAssetLongPackageName.AddUnique(LongPackageName);
		}
	}
	return OutAssetLongPackageName;
}
