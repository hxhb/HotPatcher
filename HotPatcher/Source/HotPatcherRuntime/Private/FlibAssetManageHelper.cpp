// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibAssetManageHelper.h"
#include "AssetManager/FAssetDependenciesInfo.h"
#include "AssetManager/FAssetDependenciesDetail.h"
#include "AssetManager/FFileArrayDirectoryVisitor.hpp"

#include "AssetRegistryModule.h"
#include "ARFilter.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Json.h"
#include "Templates/SharedPointer.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/AssetManager.h"
#include "AssetData.h"
#include "Resources/Version.h"
#include "HotPatcherLog.h"

bool GScanCacheOptimize = true;

// PRAGMA_DISABLE_DEPRECATION_WARNINGS
FString UFlibAssetManageHelper::PackagePathToFilename(const FString& InPackagePath)
{
	FString ResultAbsPath;
	FSoftObjectPath ObjectPath = InPackagePath;
	FString AssetAbsNotPostfix = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(ObjectPath.GetLongPackageName()));
	FString AssetName = ObjectPath.GetAssetName();
	FString SearchDir;
	{
		int32 FoundIndex;
		AssetAbsNotPostfix.FindLastChar('/', FoundIndex);
		if (FoundIndex != INDEX_NONE)
		{
			SearchDir = UKismetStringLibrary::GetSubstring(AssetAbsNotPostfix, 0, FoundIndex);
		}
	}

	TArray<FString> localFindFiles;
	IFileManager::Get().FindFiles(localFindFiles, *SearchDir, nullptr);

	for (const auto& Item : localFindFiles)
	{
		if (Item.Contains(AssetName) && Item[AssetName.Len()] == '.')
		{
			ResultAbsPath = FPaths::Combine(SearchDir, Item);
			break;
		}
	}

	return ResultAbsPath;
}


bool UFlibAssetManageHelper::FilenameToPackagePath(const FString& InAbsPath, FString& OutPackagePath)
{
	bool runState = false;
	FString LongPackageName;
	runState = FPackageName::TryConvertFilenameToLongPackageName(InAbsPath, LongPackageName);

	if (runState)
	{
		OutPackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(LongPackageName);
		runState = runState && true;
	}
	
	return runState;
}

void UFlibAssetManageHelper::UpdateAssetMangerDatabase(bool bForceRefresh)
{
#if WITH_EDITOR
	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.UpdateManagementDatabase(bForceRefresh);
#endif
}


bool UFlibAssetManageHelper::GetAssetPackageGUID(const FString& InPackagePath, FName& OutGUID)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetPackageGUID"),FColor::Red);
	bool bResult = false;
	if (InPackagePath.IsEmpty())
		return false;
	
#ifndef CUSTOM_ASSET_GUID
	const FAssetPackageData* AssetPackageData = UFlibAssetManageHelper::GetPackageDataByPackagePath(InPackagePath);
	if (AssetPackageData != NULL)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const FGuid& AssetGuid = AssetPackageData->PackageGuid;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		OutGUID = FName(AssetGuid.ToString());
		bResult = true;
	}
#else
	FString FileName;
	// FSoftObjectPath CurrentPackagePath = InPackagePath;
	// UObject* Object = CurrentPackagePath.TryLoad();
	// UPackage* Package = NULL;
	// if(Object)
	// {
	// 	Package = Object->GetPackage();
	// }
	// if(Package)
	// {
	// 	FString LongPackageName = CurrentPackagePath.GetLongPackageName();
	// 	FString Extersion = Package->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
	// 	bool bCovStatus = FPackageName::TryConvertLongPackageNameToFilename(LongPackageName,FileName,Extersion);
	// }
	
	FileName = UFlibAssetManageHelper::ConvVirtualToAbsPath(InPackagePath);

	if(!FileName.IsEmpty() && FPaths::FileExists(FileName))
	{
		FMD5Hash FileMD5Hash = FMD5Hash::HashFile(*FileName);
		FString HashValue = LexToString(FileMD5Hash);
		OutGUID = FName(HashValue);
		bResult = true;
	}
#endif
	return bResult;
}


FAssetDependenciesInfo UFlibAssetManageHelper::CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::CombineAssetDependencies"),FColor::Red);
	FAssetDependenciesInfo resault;

	auto CombineLambda = [&resault](const FAssetDependenciesInfo& InDependencies)
	{
		TArray<FString> Keys;
		InDependencies.AssetsDependenciesMap.GetKeys(Keys);
		for (const auto& Key : Keys)
		{
			if (!resault.AssetsDependenciesMap.Contains(Key))
			{
				resault.AssetsDependenciesMap.Add(Key, *InDependencies.AssetsDependenciesMap.Find(Key));
			}
			else
			{

				{
					TMap<FString,FAssetDetail>& ExistingAssetDetails = resault.AssetsDependenciesMap.Find(Key)->AssetDependencyDetails;
					TArray<FString> ExistingAssetList;
					ExistingAssetDetails.GetKeys(ExistingAssetList);

					const TMap<FString,FAssetDetail>& PaddingAssetDetails = InDependencies.AssetsDependenciesMap.Find(Key)->AssetDependencyDetails;
					TArray<FString> PaddingAssetList;
					PaddingAssetDetails.GetKeys(PaddingAssetList);

					for (const auto& PaddingDetailItem : PaddingAssetList)
					{
						if (!ExistingAssetDetails.Contains(PaddingDetailItem))
						{
							ExistingAssetDetails.Add(PaddingDetailItem,*PaddingAssetDetails.Find(PaddingDetailItem));
						}
					}
				}
			}
		}
	};

	CombineLambda(A);
	CombineLambda(B);

	return resault;
}


bool UFlibAssetManageHelper::GetAssetReference(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyType::Type>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetReference"),FColor::Red);
	bool bStatus = false;
	FString LongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(InAsset.PackagePath.ToString());
	
	{
		TArray<FAssetIdentifier> AssetIdentifier;

		FAssetIdentifier InAssetIdentifier;
		InAssetIdentifier.PackageName = FName(*LongPackageName);
		AssetIdentifier.Add(InAssetIdentifier);

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetIdentifier> ReferenceNames;
		for (const FAssetIdentifier& AssetId : AssetIdentifier)
		{
			for (const auto& AssetDepType : SearchAssetDepTypes)
			{
				TArray<FAssetIdentifier> CurrentTypeReferenceNames;
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				AssetRegistryModule.Get().GetReferencers(AssetId, CurrentTypeReferenceNames, AssetDepType);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
				for (const auto& Name : CurrentTypeReferenceNames)
				{
					if (!(Name.PackageName.ToString() == LongPackageName))
					{
						ReferenceNames.AddUnique(Name);
					}
				}
			}
			
		}

		for (const auto& RefAssetId : ReferenceNames)
		{
			FAssetDetail RefAssetDetail;
			if (UFlibAssetManageHelper::GetSpecifyAssetDetail(RefAssetId.PackageName.ToString(), RefAssetDetail))
			{
				OutRefAsset.Add(RefAssetDetail);
			}
		}
		bStatus = true;
	}
	return bStatus;
}

void UFlibAssetManageHelper::GetAssetReferenceRecursively(const FAssetDetail& InAsset,
                                                            const TArray<EAssetRegistryDependencyType::Type>&
                                                            SearchAssetDepTypes,
                                                            const TArray<FString>& SearchAssetsTypes,
                                                            TArray<FAssetDetail>& OutRefAsset)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetReferenceRecursively"),FColor::Red);
	TArray<FAssetDetail> CurrentAssetsRef;
	UFlibAssetManageHelper::GetAssetReference(InAsset,SearchAssetDepTypes,CurrentAssetsRef);

	auto MatchAssetsTypesLambda = [](const FAssetDetail& InAsset,const TArray<FString>& SearchAssetsTypes)->bool
	{
		bool bresult = false;
		if(SearchAssetsTypes.Num() > 0)
		{
			for(const auto& AssetType:SearchAssetsTypes)
			{
				if(InAsset.AssetType == FName(AssetType))
				{
					bresult = true;
					break;
				}
			}
		}
		else
		{
			bresult = true;
		}
		return bresult;
	};
	
    for(const auto& AssetRef:CurrentAssetsRef)
    {
    	if(MatchAssetsTypesLambda(AssetRef,SearchAssetsTypes))
    	{
    		if(!OutRefAsset.Contains(AssetRef))
    		{
    			OutRefAsset.AddUnique(AssetRef);
    			UFlibAssetManageHelper::GetAssetReferenceRecursively(AssetRef,SearchAssetDepTypes,SearchAssetsTypes,OutRefAsset);
    		}
    	}
    }
}

bool UFlibAssetManageHelper::GetAssetReferenceEx(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyTypeEx>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetReferenceEx"),FColor::Red);
	TArray<EAssetRegistryDependencyType::Type> local_SearchAssetDepTypes;
	for (const auto& type : SearchAssetDepTypes)
	{
		local_SearchAssetDepTypes.AddUnique(UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(type));
	}

	return UFlibAssetManageHelper::GetAssetReference(InAsset, local_SearchAssetDepTypes, OutRefAsset);
}

FAssetDetail UFlibAssetManageHelper::GetAssetDetailByPackageName(const FString& InPackageName)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetDetailByPackageName"),FColor::Red);
	FAssetDetail AssetDetail;
	UAssetManager& AssetManager = UAssetManager::Get();
	if (AssetManager.IsValid())
	{
		FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(InPackageName);
		{
			FAssetData OutAssetData;
			if (AssetManager.GetAssetDataForPath(FSoftObjectPath{ PackagePath }, OutAssetData) && OutAssetData.IsValid())
			{
				AssetDetail.PackagePath = OutAssetData.ObjectPath;
				AssetDetail.AssetType = OutAssetData.AssetClass;
				UFlibAssetManageHelper::GetAssetPackageGUID(PackagePath, AssetDetail.Guid);
			}
		}
	}
	return AssetDetail;
}



bool UFlibAssetManageHelper::GetRedirectorList(const TArray<FString>& InFilterPackagePaths, TArray<FAssetDetail>& OutRedirector)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetRedirectorList"),FColor::Red);
	TArray<FAssetData> AllAssetData;
	if (UFlibAssetManageHelper::GetAssetsData(InFilterPackagePaths, AllAssetData,true))
	{
		for (const auto& AssetDataIndex : AllAssetData)
		{
			if (AssetDataIndex.IsValid() && AssetDataIndex.IsRedirector())
			{
				FAssetDetail AssetDetail;
				UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(AssetDataIndex, AssetDetail);
				OutRedirector.Add(AssetDetail);
			}
		}
		return true;
	}
	return false;
}

bool UFlibAssetManageHelper::GetSpecifyAssetData(const FString& InLongPackageName, TArray<FAssetData>& OutAssetData, bool InIncludeOnlyOnDiskAssets)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetSpecifyAssetData"),FColor::Red);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	return AssetRegistryModule.Get().GetAssetsByPackageName(*InLongPackageName, OutAssetData, InIncludeOnlyOnDiskAssets);
}

bool UFlibAssetManageHelper::GetAssetsData(const TArray<FString>& InFilterPaths, TArray<FAssetData>& OutAssetData, bool bIncludeOnlyOnDiskAssets)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetsData"),FColor::Red);
	OutAssetData.Reset();

	FARFilter Filter;
	Filter.bIncludeOnlyOnDiskAssets = bIncludeOnlyOnDiskAssets;
	Filter.bRecursivePaths = true;
	for(const auto& FilterPackageName: InFilterPaths)
	{
		FString ValidFilterPackageName = FilterPackageName;
		while (ValidFilterPackageName.EndsWith("/"))
		{
			ValidFilterPackageName.RemoveAt(ValidFilterPackageName.Len() - 1);
		}
		Filter.PackagePaths.AddUnique(*ValidFilterPackageName);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(Filter, OutAssetData);
	
	return true;
}

// /Game 
TArray<FString> UFlibAssetManageHelper::GetAssetsByFilter(const FString& InFilter)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetsByFilter"),FColor::Red);
	TArray<FString> result;
	FString ContentPath = FPackageName::LongPackageNameToFilename(InFilter);
	TArray<FString> AssetFiles;
	TArray<FString> MapFiles;
	IFileManager::Get().FindFilesRecursive(AssetFiles, *ContentPath, *(FString(TEXT("*")) + FPackageName::GetAssetPackageExtension()), true, false);
	IFileManager::Get().FindFilesRecursive(MapFiles, *ContentPath, *(FString(TEXT("*")) + FPackageName::GetMapPackageExtension()), true, false);
	result.Append(AssetFiles);
	result.Append(MapFiles);
	return result;
}

bool UFlibAssetManageHelper::GetAssetsDataByDisk(const TArray<FString>& InFilterPaths,
	TArray<FAssetData>& OutAssetData)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetsDataByDisk"),FColor::Red);
	TSet<FName> PackageNames;
	{
		TArray<FString> AssetPaths;

		for(const auto& Filter:InFilterPaths)
		{
			AssetPaths.Append(GetAssetsByFilter(Filter));
		}
		for(auto& Asset:AssetPaths)
		{
			FString PackageName;
			if(FPackageName::TryConvertFilenameToLongPackageName(Asset,PackageName))
			{
				PackageNames.Add(FName(PackageName));
			}
		}
	}
	
	for(const auto& PackageName:PackageNames)
	{
		FAssetData CurrentAssetData;
		if(GetSingleAssetsData(PackageName.ToString(),CurrentAssetData))
		{
			OutAssetData.AddUnique(CurrentAssetData);
		}
	}
	return true;
}

bool UFlibAssetManageHelper::GetSingleAssetsData(const FString& InPackagePath, FAssetData& OutAssetData)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetSingleAssetsData"),FColor::Red);
	UAssetManager& AssetManager = UAssetManager::Get();

	return AssetManager.GetAssetDataForPath(FSoftObjectPath{ InPackagePath }, OutAssetData);
}

bool UFlibAssetManageHelper::GetAssetsDataByPackageName(const FString& InPackageName, FAssetData& OutAssetData)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetsDataByPackageName"),FColor::Red);
	FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(InPackageName);
	return UFlibAssetManageHelper::GetSingleAssetsData(PackagePath,OutAssetData);
}

bool UFlibAssetManageHelper::GetClassStringFromFAssetData(const FAssetData& InAssetData, FString& OutAssetType)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetClassStringFromFAssetData"),FColor::Red);
	bool bRunState = false;
	if (InAssetData.IsValid())
	{
		OutAssetType = InAssetData.AssetClass.ToString();
		bRunState = true;
	}
	return bRunState;
}


bool UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(const FAssetData& InAssetData, FAssetDetail& OutAssetDetail)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail"),FColor::Red);
	if (!InAssetData.IsValid())
		return false;
	FAssetDetail AssetDetail;
	AssetDetail.AssetType = FName(InAssetData.AssetClass.ToString());
	FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(InAssetData.PackageName.ToString());
	AssetDetail.PackagePath = FName(PackagePath);
	UFlibAssetManageHelper::GetAssetPackageGUID(PackagePath, AssetDetail.Guid);

	OutAssetDetail = AssetDetail;
	return !OutAssetDetail.AssetType.IsNone() && !OutAssetDetail.AssetType.IsNone() && !OutAssetDetail.AssetType.IsNone();
}

bool UFlibAssetManageHelper::GetSpecifyAssetDetail(const FString& InLongPackageName, FAssetDetail& OutAssetDetail)
{

	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetSpecifyAssetDetail"),FColor::Red);
	bool bRunStatus = false;
	TArray<FAssetData> AssetData;
	if (UFlibAssetManageHelper::GetSpecifyAssetData(InLongPackageName, AssetData, true))
	{
		if (AssetData.Num() > 0)
		{
			UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(AssetData[0], OutAssetDetail);
			bRunStatus = true;
		}
	}
	if(!bRunStatus)
	{
#if ASSET_DEPENDENCIES_DEBUG_LOG
		SCOPED_NAMED_EVENT_TCHAR(TEXT("display GetSpecifyAssetDetail failed log"),FColor::Red);
		UE_LOG(LogHotPatcher,Display,TEXT("Get %s AssetDetail failed!"),*InLongPackageName);
#endif
	}
	return bRunStatus;
}

void UFlibAssetManageHelper::FilterNoRefAssets(const TArray<FAssetDetail>& InAssetsDetail, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::FilterNoRefAssets"),FColor::Red);
	OutHasRefAssetsDetail.Reset();
	OutDontHasRefAssetsDetail.Reset();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (const auto& AssetDetail : InAssetsDetail)
	{
		FSoftObjectPath CurrentObjectSoftRef{ AssetDetail.PackagePath };
		FAssetIdentifier CurrentAssetId{ *CurrentObjectSoftRef.GetLongPackageName() };
		
		// ignore scan Map Asset reference
		{
			FAssetData CurrentAssetData;
			if (UFlibAssetManageHelper::GetSingleAssetsData(AssetDetail.PackagePath.ToString(), CurrentAssetData) && CurrentAssetData.IsValid())
			{
				if (CurrentAssetData.AssetClass == TEXT("World") || 
					CurrentAssetData.AssetClass == TEXT("MapBuildDataRegistry")
				)
				{
					OutHasRefAssetsDetail.Add(AssetDetail);
					continue;
				}
			}
		}

		
		TArray<FAssetIdentifier> CurrentAssetRefList;
		AssetRegistryModule.Get().GetReferencers(CurrentAssetId, CurrentAssetRefList);
		if (CurrentAssetRefList.Num() > 1 || (CurrentAssetRefList.Num() > 0 && !(CurrentAssetRefList[0] == CurrentAssetId)))
		{
			OutHasRefAssetsDetail.Add(AssetDetail);
		}
		else {
			OutDontHasRefAssetsDetail.Add(AssetDetail);
		}
	}
}

void UFlibAssetManageHelper::FilterNoRefAssetsWithIgnoreFilter(const TArray<FAssetDetail>& InAssetsDetail, const TArray<FString>& InIgnoreFilters, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::FilterNoRefAssetsWithIgnoreFilter"),FColor::Red);
	OutHasRefAssetsDetail.Reset();
	OutDontHasRefAssetsDetail.Reset();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (const auto& AssetDetail : InAssetsDetail)
	{
		FSoftObjectPath CurrentObjectSoftRef{ AssetDetail.PackagePath };
		FAssetIdentifier CurrentAssetId{ *CurrentObjectSoftRef.GetLongPackageName() };

		// ignore scan Map Asset reference
		{
			FAssetData CurrentAssetData;
			if (UFlibAssetManageHelper::GetSingleAssetsData(AssetDetail.PackagePath.ToString(), CurrentAssetData) && CurrentAssetData.IsValid())
			{
				if (CurrentAssetData.AssetClass == TEXT("World") ||
					CurrentAssetData.AssetClass == TEXT("MapBuildDataRegistry")
				)
				{
					bool bIsIgnoreAsset = false;
					for (const auto& IgnoreFilter : InIgnoreFilters)
					{
						if (CurrentAssetData.PackagePath.ToString().StartsWith(*IgnoreFilter))
						{
							bIsIgnoreAsset = true;
							break;
						}
					}

					if (!bIsIgnoreAsset)
					{
						OutHasRefAssetsDetail.Add(AssetDetail);
						continue;
					}
				}
			}
		}


		TArray<FAssetIdentifier> CurrentAssetRefList;
		AssetRegistryModule.Get().GetReferencers(CurrentAssetId, CurrentAssetRefList);
		if (CurrentAssetRefList.Num() > 1 || (CurrentAssetRefList.Num() > 0 && !(CurrentAssetRefList[0] == CurrentAssetId)))
		{
			OutHasRefAssetsDetail.Add(AssetDetail);
		}
		else {
			OutDontHasRefAssetsDetail.Add(AssetDetail);
		}
	}
}
bool UFlibAssetManageHelper::CombineAssetsDetailAsFAssetDepenInfo(const TArray<FAssetDetail>& InAssetsDetailList, FAssetDependenciesInfo& OutAssetInfo)
{
SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::CombineAssetsDetailAsFAssetDepenInfo"),FColor::Red);
	FAssetDependenciesInfo result;

	for (const auto& AssetDetail : InAssetsDetailList)
	{
		FString CurrAssetModuleName = UFlibAssetManageHelper::GetAssetBelongModuleName(AssetDetail.PackagePath.ToString());
		FSoftObjectPath CurrAssetObjectPath(AssetDetail.PackagePath);
		FString CurrAssetLongPackageName = CurrAssetObjectPath.GetLongPackageName();
		if (!result.AssetsDependenciesMap.Contains(CurrAssetModuleName))
		{
			FAssetDependenciesDetail AssetDependenciesDetail{ CurrAssetModuleName,TMap<FString,FAssetDetail>{ {CurrAssetLongPackageName,AssetDetail} } };
			result.AssetsDependenciesMap.Add(CurrAssetModuleName, AssetDependenciesDetail);
		}
		else
		{
			FAssetDependenciesDetail& CurrentCategory = *result.AssetsDependenciesMap.Find(CurrAssetModuleName);
			
			if (!result.AssetsDependenciesMap.Contains(CurrAssetLongPackageName))
			{
				CurrentCategory.AssetDependencyDetails.Add(CurrAssetLongPackageName,AssetDetail);
			}
		}
	
	}
	OutAssetInfo = result;

	return true;
}



void UFlibAssetManageHelper::GetAllInValidAssetInProject(FAssetDependenciesInfo InAllDependencies, TArray<FString> &OutInValidAsset,TArray<FString> InIgnoreModules)
{
SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAllInValidAssetInProject"),FColor::Red);
	TArray<FString> Keys;
	InAllDependencies.AssetsDependenciesMap.GetKeys(Keys);

	for (const auto& ModuleItem : Keys)
	{
		// ignore search /Script Asset
		if (InIgnoreModules.Contains(ModuleItem))
			continue;
		FAssetDependenciesDetail ModuleDependencies = InAllDependencies.AssetsDependenciesMap[ModuleItem];

		TArray<FString> ModuleAssetList;
		ModuleDependencies.AssetDependencyDetails.GetKeys(ModuleAssetList);
		for (const auto& AssetLongPackageName : ModuleAssetList)
		{
			FString AssetPackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(AssetLongPackageName);
			// UE_LOG(LogHotPatcher, Log, TEXT("Asset %s"), *AssetPackagePath);
			FString AssetAbsPath = UFlibAssetManageHelper::PackagePathToFilename(AssetPackagePath);
			if (!FPaths::FileExists(AssetAbsPath))
			{
				OutInValidAsset.Add(AssetLongPackageName);
			}
		}
	}
}
const FAssetPackageData* UFlibAssetManageHelper::GetPackageDataByPackagePath(const FString& InPackagePath)
{

	if (InPackagePath.IsEmpty())
		return NULL;
	if (!InPackagePath.IsEmpty())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FString TargetLongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(InPackagePath);

		if(FPackageName::DoesPackageExist(TargetLongPackageName))
		{
#if ENGINE_MAJOR_VERSION > 4 && ENGINE_MINOR_VERSION > 0
			TOptional<FAssetPackageData> PackageDataOpt = AssetRegistryModule.Get().GetAssetPackageDataCopy(*TargetLongPackageName);
			const FAssetPackageData* PackageData = &PackageDataOpt.GetValue();
#else
			const FAssetPackageData* PackageData = AssetRegistryModule.Get().GetAssetPackageData(*TargetLongPackageName);
#endif
			FAssetPackageData* AssetPackageData = const_cast<FAssetPackageData*>(PackageData);
			if (AssetPackageData != nullptr)
			{
				return AssetPackageData;
			}
		}
		else
		{
			UE_LOG(LogHotPatcher,Warning,TEXT("GetPackageDataByPackagePath %s Failed!"),*InPackagePath);
		}
	}

	return NULL;
}

bool UFlibAssetManageHelper::ConvLongPackageNameToCookedPath(const FString& InProjectAbsDir, const FString& InPlatformName, const FString& InLongPackageName, TArray<FString>& OutCookedAssetPath, TArray<FString>& OutCookedAssetRelativePath)
{
	if (!FPaths::DirectoryExists(InProjectAbsDir)/* || !IsValidPlatform(InPlatformName)*/)
		return false;

	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	FString CookedRootDir = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"), InPlatformName);
	FString ProjectName = FApp::GetProjectName();
	FString AssetPackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(InLongPackageName);
	FString AssetAbsPath = UFlibAssetManageHelper::PackagePathToFilename(AssetPackagePath);

	FString AssetModuleName;
	GetModuleNameByRelativePath(InLongPackageName,AssetModuleName);

	bool bIsEngineModule = false;
	FString AssetBelongModuleBaseDir;
	FString AssetCookedNotPostfixPath;
	{

		if (UFlibAssetManageHelper::GetEnableModuleAbsDir(AssetModuleName, AssetBelongModuleBaseDir))
		{
			if (AssetBelongModuleBaseDir.Contains(EngineAbsDir))
				bIsEngineModule = true;
		}

		FString AssetCookedRelativePath;
		if (bIsEngineModule)
		{
			AssetCookedRelativePath = TEXT("Engine") / UKismetStringLibrary::GetSubstring(AssetAbsPath, EngineAbsDir.Len() - 1, AssetAbsPath.Len() - EngineAbsDir.Len());
		}
		else
		{
			AssetCookedRelativePath = ProjectName / UKismetStringLibrary::GetSubstring(AssetAbsPath, InProjectAbsDir.Len() - 1, AssetAbsPath.Len() - InProjectAbsDir.Len());
		}

		// remove .uasset / .umap postfix
		{
			int32 lastDotIndex = 0;
			AssetCookedRelativePath.FindLastChar('.', lastDotIndex);
			AssetCookedRelativePath.RemoveAt(lastDotIndex, AssetCookedRelativePath.Len() - lastDotIndex);
		}

		AssetCookedNotPostfixPath = FPaths::Combine(CookedRootDir, AssetCookedRelativePath);
	}

	FFileArrayDirectoryVisitor FileVisitor;
	FString SearchDir;
	{
		int32 lastSlashIndex;
		AssetCookedNotPostfixPath.FindLastChar('/', lastSlashIndex);
		SearchDir = UKismetStringLibrary::GetSubstring(AssetCookedNotPostfixPath, 0, lastSlashIndex);
	}
	IFileManager::Get().IterateDirectory(*SearchDir, FileVisitor);
	for (const auto& FileItem : FileVisitor.Files)
	{
		if (FileItem.Contains(AssetCookedNotPostfixPath) && FileItem[AssetCookedNotPostfixPath.Len()] == '.')
		{
			OutCookedAssetPath.Add(FileItem);
			{
				FString AssetCookedRelativePath = UKismetStringLibrary::GetSubstring(FileItem, CookedRootDir.Len() + 1, FileItem.Len() - CookedRootDir.Len());
				OutCookedAssetRelativePath.Add(FPaths::Combine(TEXT("../../../"), AssetCookedRelativePath));
			}
		}
	}
	return true;
}


bool UFlibAssetManageHelper::MakePakCommandFromAssetDependencies(
	const FString& InProjectDir, 
	const FString& InPlatformName, 
	const FAssetDependenciesInfo& InAssetDependencies, 
	//const TArray<FString> &InCookParams, 
	TArray<FString>& OutCookCommand, 
	TFunction<void(const TArray<FString>&,const TArray<FString>&, const FString&, const FString&)> InReceivePakCommand,
	TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::MakePakCommandFromAssetDependencies"),FColor::Red);
	if (!FPaths::DirectoryExists(InProjectDir) /*|| !UFlibAssetManageHelper::IsValidPlatform(InPlatformName)*/)
		return false;
	OutCookCommand.Empty();
	// TArray<FString> resault;
	TArray<FString> Keys;
	InAssetDependencies.AssetsDependenciesMap.GetKeys(Keys);

	for (const auto& Key : Keys)
	{
		if (Key.Equals(TEXT("Script")))
			continue;
		TArray<FString> AssetList;
		InAssetDependencies.AssetsDependenciesMap.Find(Key)->AssetDependencyDetails.GetKeys(AssetList);
		for (const auto& AssetLongPackageName : AssetList)
		{
			TArray<FString> FinalCookedCommand;
			if (UFlibAssetManageHelper::MakePakCommandFromLongPackageName(InProjectDir, InPlatformName, AssetLongPackageName, /*InCookParams, */FinalCookedCommand,InReceivePakCommand,IsIoStoreAsset))
			{
				OutCookCommand.Append(FinalCookedCommand);
			}
		}
	}
	return true;
}


bool UFlibAssetManageHelper::MakePakCommandFromLongPackageName(
	const FString& InProjectDir, 
	const FString& InPlatformName, 
	const FString& InAssetLongPackageName, 
	//const TArray<FString> &InCookParams, 
	TArray<FString>& OutCookCommand,
	TFunction<void(const TArray<FString>&,const TArray<FString>&, const FString&, const FString&)> InReceivePakCommand,
	TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::MakePakCommandFromLongPackageName"),FColor::Red);
	OutCookCommand.Empty();
	bool bStatus = false;
	TArray<FString> CookedAssetAbsPath;
	TArray<FString> CookedAssetRelativePath;
	TArray<FString> FinalCookedPakCommand;
	TArray<FString> FinalCookedIoStoreCommand;
	if (UFlibAssetManageHelper::ConvLongPackageNameToCookedPath(InProjectDir, InPlatformName, InAssetLongPackageName, CookedAssetAbsPath, CookedAssetRelativePath))
	{
		if (UFlibAssetManageHelper::CombineCookedAssetCommand(CookedAssetAbsPath, CookedAssetRelativePath,/* InCookParams,*/ FinalCookedPakCommand,FinalCookedIoStoreCommand,IsIoStoreAsset))
		{
			if (!!CookedAssetRelativePath.Num() && !!FinalCookedPakCommand.Num())
			{
				InReceivePakCommand(FinalCookedPakCommand,FinalCookedIoStoreCommand, CookedAssetRelativePath[0],InAssetLongPackageName);
				OutCookCommand.Append(FinalCookedPakCommand);
				bStatus = true;
			}
		}
	}
	return bStatus;
}

bool UFlibAssetManageHelper::CombineCookedAssetCommand(
	const TArray<FString> &InAbsPath,
	const TArray<FString>& InRelativePath,
	//const TArray<FString>& InParams,
	TArray<FString>& OutPakCommand,
	TArray<FString>& OutIoStoreCommand,
	TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset
)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::CombineCookedAssetCommand"),FColor::Red);
	if (!(!!InAbsPath.Num() && !!InRelativePath.Num()))
	{
		return false;
	}

	OutPakCommand.Empty();
	if (InAbsPath.Num() != InRelativePath.Num())
		return false;
	int32 AssetNum = InAbsPath.Num();
	for (int32 index = 0; index < AssetNum; ++index)
	{
		if(InAbsPath[index].EndsWith(TEXT(".patch")))
			continue;;
		FString CurrentCommand = TEXT("\"") + InAbsPath[index] + TEXT("\" \"") + InRelativePath[index] + TEXT("\"");
		// for (const auto& Param : InParams)
		// {
		// 	CurrentCommand += TEXT(" ") + Param;
		// }
		if(!IsIoStoreAsset(InAbsPath[index]))
			OutPakCommand.Add(CurrentCommand);
		else
			OutIoStoreCommand.Add(CurrentCommand);
	}
	return true;
}

bool UFlibAssetManageHelper::ExportCookPakCommandToFile(const TArray<FString>& InCommand, const FString& InFile)
{
	return FFileHelper::SaveStringArrayToFile(InCommand, *InFile);
}

bool UFlibAssetManageHelper::SaveStringToFile(const FString& InFile, const FString& InString)
{
	return FFileHelper::SaveStringToFile(InString, *InFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool UFlibAssetManageHelper::LoadFileToString(const FString& InFile, FString& OutString)
{
	return FFileHelper::LoadFileToString(OutString, *InFile);
}

bool UFlibAssetManageHelper::GetPluginModuleAbsDir(const FString& InPluginModuleName, FString& OutPath)
{
	bool bFindResault = false;
	TSharedPtr<IPlugin> FoundModule = IPluginManager::Get().FindPlugin(InPluginModuleName);

	if (FoundModule.IsValid())
	{
		bFindResault = true;
		OutPath = FPaths::ConvertRelativePathToFull(FoundModule->GetBaseDir());
	}
	return bFindResault;
}

void UFlibAssetManageHelper::GetAllEnabledModuleName(TMap<FString, FString>& OutModules)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAllEnabledModuleName"),FColor::Red);
	OutModules.Add(TEXT("Game"), FPaths::ProjectDir());
	OutModules.Add(TEXT("Engine"), FPaths::EngineDir());

	TArray<TSharedRef<IPlugin>> AllPlugin = IPluginManager::Get().GetEnabledPlugins();

	for (const auto& PluginItem : AllPlugin)
	{
		OutModules.Add(PluginItem.Get().GetName(), PluginItem.Get().GetBaseDir());
	}
}


bool UFlibAssetManageHelper::ModuleIsEnabled(const FString& InModuleName)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::ModuleIsEnabled"),FColor::Red);
	TMap<FString,FString> AllEnabledModules;
	UFlibAssetManageHelper::GetAllEnabledModuleName(AllEnabledModules);
	TArray<FString> AllEnableModuleNames;
	AllEnabledModules.GetKeys(AllEnableModuleNames);
	return AllEnabledModules.Contains(InModuleName);
}

bool UFlibAssetManageHelper::GetModuleNameByRelativePath(const FString& InRelativePath, FString& OutModuleName)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetModuleNameByRelativePath"),FColor::Red);
	if (InRelativePath.IsEmpty()) return false;
	FString BelongModuleName = InRelativePath;
	{
		BelongModuleName.RemoveFromStart(TEXT("/"));
		int32 secondSlashIndex = -1;
		if(BelongModuleName.FindChar('/', secondSlashIndex))
			BelongModuleName = UKismetStringLibrary::GetSubstring(BelongModuleName, 0, secondSlashIndex);
	}
	OutModuleName = BelongModuleName;
	return true;
}

bool UFlibAssetManageHelper::GetEnableModuleAbsDir(const FString& InModuleName, FString& OutPath)
{
	if (InModuleName.Equals(TEXT("Engine")))
	{
		OutPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
		return true;
	}
	if (InModuleName.Equals(TEXT("Game")) || InModuleName.Equals(FApp::GetProjectName()))
	{
		OutPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		return true;
	}
	return UFlibAssetManageHelper::GetPluginModuleAbsDir(InModuleName, OutPath);
}

FString UFlibAssetManageHelper::GetAssetBelongModuleName(const FString& InAssetRelativePath)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetBelongModuleName"),FColor::Red);
	FString LongDependentPackageName = InAssetRelativePath;

	int32 BelongModuleNameEndIndex = LongDependentPackageName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1);
	FString BelongModuleName = UKismetStringLibrary::GetSubstring(LongDependentPackageName, 1, BelongModuleNameEndIndex - 1);// (LongDependentPackageName, BelongModuleNameEndIndex);

	return BelongModuleName;
}


bool UFlibAssetManageHelper::ConvRelativeDirToAbsDir(const FString& InRelativePath, FString& OutAbsPath)
{
	bool bRunStatus = false;
	FString BelongModuleName;
	if (UFlibAssetManageHelper::GetModuleNameByRelativePath(InRelativePath, BelongModuleName))
	{
		if (UFlibAssetManageHelper::ModuleIsEnabled(BelongModuleName))
		{
			FString ModuleAbsPath;
			if (!UFlibAssetManageHelper::GetEnableModuleAbsDir(BelongModuleName, ModuleAbsPath))
				return false;

			FString RelativeToModule = InRelativePath.Replace(*BelongModuleName, TEXT("Content"));
			RelativeToModule = RelativeToModule.StartsWith(TEXT("/")) ? RelativeToModule.Right(RelativeToModule.Len() - 1) : RelativeToModule;
			FString FinalFilterPath = FPaths::Combine(ModuleAbsPath, RelativeToModule);
			FPaths::MakeStandardFilename(FinalFilterPath);
			if (FPaths::DirectoryExists(FinalFilterPath))
			{
				OutAbsPath = FinalFilterPath;
				bRunStatus = true;
			}
		}
	}
	return bRunStatus;
}

bool UFlibAssetManageHelper::FindFilesRecursive(const FString& InStartDir, TArray<FString>& OutFileList, bool InRecursive)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::FindFilesRecursive"),FColor::Red);
	TArray<FString> CurrentFolderFileList;
	if (!FPaths::DirectoryExists(InStartDir))
		return false;

	FFileArrayDirectoryVisitor FileVisitor;
	IFileManager::Get().IterateDirectoryRecursively(*InStartDir, FileVisitor);

	OutFileList.Append(FileVisitor.Files);

	return true;
}

uint32 UFlibAssetManageHelper::ParserAssetDependenciesInfoNumber(const FAssetDependenciesInfo& AssetDependenciesInfo, TMap<FString, uint32> ModuleAssetsNumMaps)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::ParserAssetDependenciesInfoNumber"),FColor::Red);
	uint32 TotalAssetNum = 0;
	TArray<FString> ModuleCategorys;
	AssetDependenciesInfo.AssetsDependenciesMap.GetKeys(ModuleCategorys);
	for(const auto& ModuleKey:ModuleCategorys)
	{
		TArray<FString> AssetKeys;
		AssetDependenciesInfo.AssetsDependenciesMap.Find(ModuleKey)->AssetDependencyDetails.GetKeys(AssetKeys);
		ModuleAssetsNumMaps.Add(ModuleKey,AssetKeys.Num());
		TotalAssetNum+=AssetKeys.Num();
	}
	return TotalAssetNum;
}

FString UFlibAssetManageHelper::ParserModuleAssetsNumMap(const TMap<FString, uint32>& InMap)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::ParserModuleAssetsNumMap"),FColor::Red);
	FString result;
	TArray<FString> Keys;
	InMap.GetKeys(Keys);

	for(const auto& Key:Keys)
	{
		result += FString::Printf(TEXT("%s\t%d\n"),*Key,*InMap.Find(Key));
	}
	return result;
}

EAssetRegistryDependencyType::Type UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(const EAssetRegistryDependencyTypeEx& InType)
{
	return static_cast<EAssetRegistryDependencyType::Type>((uint8)(InType));
}

void UFlibAssetManageHelper::GetAssetDataInPaths(const TArray<FString>& Paths, TArray<FAssetData>& OutAssetData)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::GetAssetDataInPaths"),FColor::Red);
	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (const FString& Path : Paths)
	{
		new (Filter.PackagePaths) FName(*Path);
	}
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	AssetRegistryModule.Get().GetAssets(Filter, OutAssetData);
}

FString UFlibAssetManageHelper::LongPackageNameToPackagePath(const FString& InLongPackageName)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::LongPackageNameToPackagePath"),FColor::Red);
	if(InLongPackageName.Contains(TEXT(".")))
	{
		UE_LOG(LogHotPatcher,Warning,TEXT("LongPackageNameToPackagePath %s alway is PackagePath!"),*InLongPackageName);
		return InLongPackageName;
	}
	FString AssetName;
	{
		int32 FoundIndex;
		InLongPackageName.FindLastChar('/', FoundIndex);
		if (FoundIndex != INDEX_NONE)
		{
			AssetName = UKismetStringLibrary::GetSubstring(InLongPackageName, FoundIndex + 1, InLongPackageName.Len() - FoundIndex);
		}
	}
	FString OutPackagePath = InLongPackageName + TEXT(".") + AssetName;
	return OutPackagePath;
}

FString UFlibAssetManageHelper::PackagePathToLongPackageName(const FString& PackagePath)
{
	SCOPED_NAMED_EVENT_TCHAR(TEXT("UFlibAssetManageHelper::PackagePathToLongPackageName"),FColor::Red);
	FSoftObjectPath ObjectPath = PackagePath;
	return ObjectPath.GetLongPackageName();
}

void UFlibAssetManageHelper::ExcludeContentForAssetDependenciesDetail(FAssetDependenciesInfo& AssetDependencies,const TArray<FString>& ExcludeRules)
{
	auto ExcludeEditorContent = [&ExcludeRules](TMap<FString,FAssetDependenciesDetail>& AssetCategorys)
	{
		TArray<FString> Keys;
		AssetCategorys.GetKeys(Keys);
		for(const auto& Key:Keys)
		{
			FAssetDependenciesDetail&  ModuleAssetList = *AssetCategorys.Find(Key);
			TArray<FString> AssetKeys;
			
			ModuleAssetList.AssetDependencyDetails.GetKeys(AssetKeys);
			for(const auto& AssetKey:AssetKeys)
			{
				bool customStartWith = false;
				FString MatchRule;
				for(const auto& Rule:ExcludeRules)
				{
					if(AssetKey.StartsWith(Rule))
					{
						MatchRule = Rule;
						customStartWith = true;
						break;
					}
				}
				if( customStartWith )
				{
					UE_LOG(LogHotPatcher,Display,TEXT("remove %s in AssetDependencies(match exclude rule %s)"),*AssetKey,*MatchRule);
					ModuleAssetList.AssetDependencyDetails.Remove(AssetKey);
				}
			}
		}
	};
	ExcludeEditorContent(AssetDependencies.AssetsDependenciesMap);
}

TArray<FString> UFlibAssetManageHelper::DirectoryPathsToStrings(const TArray<FDirectoryPath>& DirectoryPaths)
{
	TArray<FString> Path;
	for(const auto& DirPath:DirectoryPaths)
	{
		Path.AddUnique(NormalizeContentDir(DirPath.Path));
	}
	return Path;
}

TArray<FString> UFlibAssetManageHelper::SoftObjectPathsToStrings(const TArray<FSoftObjectPath>& SoftObjectPaths)
{
	TArray<FString> result;
	for(const auto &Asset:SoftObjectPaths)
	{
		result.Add(Asset.GetLongPackageName());
	}
	return result;
}

FString UFlibAssetManageHelper::NormalizeContentDir(const FString& Dir)
{
	FString result = Dir;
	if(!Dir.EndsWith(TEXT("/")))
	{
		result = FString::Printf(TEXT("%s/"),*Dir);
	}
	return result;
}
// PRAGMA_ENABLE_DEPRECATION_WARNINGS
