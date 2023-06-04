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
#include "UObject/ConstructorHelpers.h"
#include "Resources/Version.h"
#include "HotPatcherLog.h"
#include "HotPatcherRuntime.h"
#include "Async/ParallelFor.h"
#include "Templates/HotPatcherTemplateHelper.hpp"
#include "UObject/MetaData.h"
#include "UObject/UObjectHash.h"
#include "Misc/EngineVersionComparison.h"

bool UFlibAssetManageHelper::bIncludeOnlyOnDiskAssets = !GForceSingleThread;

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

FString UFlibAssetManageHelper::LongPackageNameToFilename(const FString& InLongPackageName)
{
	return UFlibAssetManageHelper::PackagePathToFilename(UFlibAssetManageHelper::LongPackageNameToPackagePath(InLongPackageName));
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
	SCOPED_NAMED_EVENT_TEXT("UpdateAssetMangerDatabase",FColor::Red);
#if WITH_EDITOR
	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.UpdateManagementDatabase(bForceRefresh);
#endif
}


bool UFlibAssetManageHelper::GetAssetPackageGUID(const FString& InPackageName, FName& OutGUID)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetPackageGUID",FColor::Red);
	bool bResult = false;
	if (InPackageName.IsEmpty())
		return false;
	
#ifndef CUSTOM_ASSET_GUID
	const FAssetPackageData* AssetPackageData = UFlibAssetManageHelper::GetPackageDataByPackageName(InPackageName);
	if (AssetPackageData != NULL)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const FGuid& AssetGuid = AssetPackageData->PackageGuid;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		OutGUID = FName(*AssetGuid.ToString());
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

	FileName = UFlibAssetManageHelper::LongPackageNameToFilename(InPackageName);
	
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

FSoftObjectPath UFlibAssetManageHelper::CreateSoftObjectPathByPackage(UPackage* Package)
{
	FString AssetPathName = Package->GetPathName();	
	FSoftObjectPath Path(UFlibAssetManageHelper::LongPackageNameToPackagePath(AssetPathName));
	return Path;
}

FName UFlibAssetManageHelper::GetAssetTypeByPackage(UPackage* Package)
{
	return UFlibAssetManageHelper::GetAssetType(CreateSoftObjectPathByPackage(Package));
}


FAssetDependenciesInfo UFlibAssetManageHelper::CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::CombineAssetDependencies",FColor::Red);
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


bool UFlibAssetManageHelper::GetAssetReferenceByLongPackageName(const FString& LongPackageName,
	const TArray<EAssetRegistryDependencyType::Type>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset)
{
	bool bStatus = false;
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


bool UFlibAssetManageHelper::GetAssetReference(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyType::Type>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetReference",FColor::Red);
	FString LongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(InAsset.PackagePath.ToString());
	return UFlibAssetManageHelper::GetAssetReferenceByLongPackageName(LongPackageName,SearchAssetDepTypes,OutRefAsset);
}

void UFlibAssetManageHelper::GetAssetReferenceRecursively(const FAssetDetail& InAsset,
                                                          const TArray<EAssetRegistryDependencyType::Type>&
                                                          SearchAssetDepTypes,
                                                          const TArray<FString>& SearchAssetsTypes,
                                                          TArray<FAssetDetail>& OutRefAsset, bool bRecursive)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetReferenceRecursively",FColor::Red);
	TArray<FAssetDetail> CurrentAssetsRef;
	UFlibAssetManageHelper::GetAssetReference(InAsset,SearchAssetDepTypes,CurrentAssetsRef);

	auto MatchAssetsTypesLambda = [](const FAssetDetail& InAsset,const TArray<FString>& SearchAssetsTypes)->bool
	{
		bool bresult = false;
		if(SearchAssetsTypes.Num() > 0)
		{
			for(const auto& AssetType:SearchAssetsTypes)
			{
				if(InAsset.AssetType == FName(*AssetType))
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
    			if(bRecursive)
    			{
    				UFlibAssetManageHelper::GetAssetReferenceRecursively(AssetRef,SearchAssetDepTypes,SearchAssetsTypes,OutRefAsset, bRecursive);
    			}
    		}
    	}
    }
}

bool UFlibAssetManageHelper::GetAssetReferenceEx(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyTypeEx>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetReferenceEx",FColor::Red);
	TArray<EAssetRegistryDependencyType::Type> local_SearchAssetDepTypes;
	for (const auto& type : SearchAssetDepTypes)
	{
		local_SearchAssetDepTypes.AddUnique(UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(type));
	}

	return UFlibAssetManageHelper::GetAssetReference(InAsset, local_SearchAssetDepTypes, OutRefAsset);
}

FName UFlibAssetManageHelper::GetAssetType(FSoftObjectPath SoftObjectPath)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetTypeByPackageName",FColor::Red);
	UAssetManager& AssetManager = UAssetManager::Get();
	FAssetData OutAssetData = AssetManager.GetAssetRegistry().GetAssetByObjectPath(
#if !UE_VERSION_OLDER_THAN(5,0,0)
	SoftObjectPath,
#else
*SoftObjectPath.GetAssetPathString(),
#endif
	bIncludeOnlyOnDiskAssets);
	// UAssetManager::Get().GetAssetDataForPath(SoftObjectPath, OutAssetData) && OutAssetData.IsValid();
	
	return UFlibAssetManageHelper::GetAssetDataClasses(OutAssetData);
}

FAssetDetail UFlibAssetManageHelper::GetAssetDetailByPackageName(const FString& InPackageName)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetDetailByPackageName",FColor::Red);
	FAssetDetail AssetDetail;
	UAssetManager& AssetManager = UAssetManager::Get();
	if (AssetManager.IsValid())
	{
		FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(InPackageName);
		{
			FAssetData OutAssetData = AssetManager.GetAssetRegistry().GetAssetByObjectPath(
#if !UE_VERSION_OLDER_THAN(5,0,0)
			FSoftObjectPath(PackagePath),
#else
			FName(*PackagePath),
#endif
			bIncludeOnlyOnDiskAssets);
			if(OutAssetData.IsValid())
			{
				AssetDetail.PackagePath = UFlibAssetManageHelper::GetObjectPathByAssetData(OutAssetData);
				AssetDetail.AssetType = UFlibAssetManageHelper::GetAssetDataClasses(OutAssetData);
#if ENGINE_MAJOR_VERSION > 4				
				UFlibAssetManageHelper::GetAssetPackageGUID(AssetDetail.PackagePath.ToString(), AssetDetail.Guid);
#else
				UFlibAssetManageHelper::GetAssetPackageGUID(InPackageName, AssetDetail.Guid);
#endif				
			}
		}
	}
	return AssetDetail;
}

bool UFlibAssetManageHelper::GetRedirectorList(const TArray<FString>& InFilterPackagePaths, TArray<FAssetDetail>& OutRedirector)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetRedirectorList",FColor::Red);
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

bool UFlibAssetManageHelper::IsRedirector(const FAssetDetail& Src,FAssetDetail& Out)
{
	SCOPED_NAMED_EVENT_TEXT("IsRedirector",FColor::Red);
	bool bIsRedirector = false;
	FAssetData AssetData;
	if (UFlibAssetManageHelper::GetSingleAssetsData(Src.PackagePath.ToString(), AssetData))
	{
		if ((AssetData.IsValid() && AssetData.IsRedirector()) ||
			UObjectRedirector::StaticClass()->GetFName() == Src.AssetType ||
			Src.PackagePath != UFlibAssetManageHelper::GetObjectPathByAssetData(AssetData)
			)
		{
			FAssetDetail AssetDetail;
			UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(AssetData, AssetDetail);
			Out = AssetDetail;
			bIsRedirector = true;
		}
	}
	return bIsRedirector;
}

bool UFlibAssetManageHelper::GetSpecifyAssetData(const FString& InLongPackageName, TArray<FAssetData>& OutAssetData, bool InIncludeOnlyOnDiskAssets)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetSpecifyAssetData",FColor::Red);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	return AssetRegistryModule.Get().GetAssetsByPackageName(*InLongPackageName, OutAssetData, InIncludeOnlyOnDiskAssets);
}

bool UFlibAssetManageHelper::GetAssetsData(const TArray<FString>& InFilterPaths, TArray<FAssetData>& OutAssetData, bool bLocalIncludeOnlyOnDiskAssets)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetsData",FColor::Red);
	OutAssetData.Reset();

	FARFilter Filter;
#if (ENGINE_MAJOR_VERSION > 4) || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
	Filter.WithoutPackageFlags = PKG_FilterEditorOnly;
#endif
	Filter.bIncludeOnlyOnDiskAssets = bLocalIncludeOnlyOnDiskAssets;
	Filter.bRecursivePaths = true;
	for(const auto& FilterPackageName: InFilterPaths)
	{
		FString ValidFilterPackageName = FilterPackageName;

#if ENGINE_MAJOR_VERSION > 4
		if(ValidFilterPackageName.StartsWith(TEXT("/All/"),ESearchCase::IgnoreCase))
		{
			ValidFilterPackageName.RemoveFromStart(TEXT("/All"),ESearchCase::IgnoreCase);
		}
#endif
		
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetsByFilter",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetsDataByDisk",FColor::Red);
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
				PackageNames.Add(FName(*PackageName));
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetSingleAssetsData",FColor::Red);
	UAssetManager& AssetManager = UAssetManager::Get();

	OutAssetData = AssetManager.GetAssetRegistry().GetAssetByObjectPath(
#if !UE_VERSION_OLDER_THAN(5,0,0)
	FSoftObjectPath(InPackagePath),
#else
	FName(*InPackagePath),
#endif
	bIncludeOnlyOnDiskAssets);
	
	return OutAssetData.IsValid();//AssetManager.GetAssetDataForPath(FSoftObjectPath{ InPackagePath }, OutAssetData);
}

bool UFlibAssetManageHelper::GetAssetsDataByPackageName(const FString& InPackageName, FAssetData& OutAssetData)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetsDataByPackageName",FColor::Red);
	FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(InPackageName);
	return UFlibAssetManageHelper::GetSingleAssetsData(PackagePath,OutAssetData);
}

bool UFlibAssetManageHelper::GetClassStringFromFAssetData(const FAssetData& InAssetData, FString& OutAssetType)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetClassStringFromFAssetData",FColor::Red);
	bool bRunState = false;
	if (InAssetData.IsValid())
	{
		OutAssetType = UFlibAssetManageHelper::GetAssetDataClasses(InAssetData).ToString();
		bRunState = true;
	}
	return bRunState;
}


bool UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail(const FAssetData& InAssetData, FAssetDetail& OutAssetDetail)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::ConvFAssetDataToFAssetDetail",FColor::Red);
	if (!InAssetData.IsValid())
		return false;
	FAssetDetail AssetDetail;
	AssetDetail.AssetType = UFlibAssetManageHelper::GetAssetDataClasses(InAssetData);
	FString PackageName = InAssetData.PackageName.ToString();
	FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(PackageName);
	AssetDetail.PackagePath = FName(*PackagePath);
#if ENGINE_MAJOR_VERSION > 4	
	UFlibAssetManageHelper::GetAssetPackageGUID(PackagePath, AssetDetail.Guid);
#else
	UFlibAssetManageHelper::GetAssetPackageGUID(PackageName, AssetDetail.Guid);
#endif

	OutAssetDetail = AssetDetail;
	return OutAssetDetail.IsValid();
}

bool UFlibAssetManageHelper::GetSpecifyAssetDetail(const FString& InLongPackageName, FAssetDetail& OutAssetDetail)
{

	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetSpecifyAssetDetail",FColor::Red);
	bool bRunStatus = false;
	TArray<FAssetData> AssetData;
	if (UFlibAssetManageHelper::GetSpecifyAssetData(InLongPackageName, AssetData, bIncludeOnlyOnDiskAssets))
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
		SCOPED_NAMED_EVENT_TEXT("display GetSpecifyAssetDetail failed log",FColor::Red);
		UE_LOG(LogHotPatcher,Log,TEXT("Get %s AssetDetail failed!"),*InLongPackageName);
#endif
	}
	return bRunStatus;
}

void UFlibAssetManageHelper::FilterNoRefAssets(const TArray<FAssetDetail>& InAssetsDetail, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::FilterNoRefAssets",FColor::Red);
	OutHasRefAssetsDetail.Reset();
	OutDontHasRefAssetsDetail.Reset();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (const auto& AssetDetail : InAssetsDetail)
	{
		FSoftObjectPath CurrentObjectSoftRef{ AssetDetail.PackagePath.ToString() };
		FAssetIdentifier CurrentAssetId{ *CurrentObjectSoftRef.GetLongPackageName() };
		
		// ignore scan Map Asset reference
		{
			FAssetData CurrentAssetData;
			if (UFlibAssetManageHelper::GetSingleAssetsData(AssetDetail.PackagePath.ToString(), CurrentAssetData) && CurrentAssetData.IsValid())
			{
				FName ClassName = UFlibAssetManageHelper::GetAssetDataClasses(CurrentAssetData);
				if (ClassName == TEXT("World") || 
					ClassName == TEXT("MapBuildDataRegistry")
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::FilterNoRefAssetsWithIgnoreFilter",FColor::Red);
	OutHasRefAssetsDetail.Reset();
	OutDontHasRefAssetsDetail.Reset();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (const auto& AssetDetail : InAssetsDetail)
	{
		FSoftObjectPath CurrentObjectSoftRef{ AssetDetail.PackagePath.ToString() };
		FAssetIdentifier CurrentAssetId{ *CurrentObjectSoftRef.GetLongPackageName() };

		// ignore scan Map Asset reference
		{
			FAssetData CurrentAssetData;
			if (UFlibAssetManageHelper::GetSingleAssetsData(AssetDetail.PackagePath.ToString(), CurrentAssetData) && CurrentAssetData.IsValid())
			{
				FName ClassName = UFlibAssetManageHelper::GetAssetDataClasses(CurrentAssetData);
				if (ClassName == TEXT("World") ||
					ClassName == TEXT("MapBuildDataRegistry")
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
SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::CombineAssetsDetailAsFAssetDepenInfo",FColor::Red);
	FAssetDependenciesInfo result;

	for (const auto& AssetDetail : InAssetsDetailList)
	{
		FString CurrAssetModuleName = UFlibAssetManageHelper::GetAssetBelongModuleName(AssetDetail.PackagePath.ToString());
		FSoftObjectPath CurrAssetObjectPath(AssetDetail.PackagePath.ToString());
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
SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAllInValidAssetInProject",FColor::Red);
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
			FString AssetAbsPath = UFlibAssetManageHelper::LongPackageNameToFilename(AssetLongPackageName);
			if (!FPaths::FileExists(AssetAbsPath))
			{
				OutInValidAsset.Add(AssetLongPackageName);
			}
		}
	}
}
#pragma warning(push)
#pragma warning(disable:4172)
FAssetPackageData* UFlibAssetManageHelper::GetPackageDataByPackageName(const FString& InPackageName)
{
	FAssetPackageData* AssetPackageData = nullptr;
	if (InPackageName.IsEmpty())
		return NULL;
	if (!InPackageName.IsEmpty())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		// FString TargetLongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(InPackageName);
		const FString& TargetLongPackageName = InPackageName;
#if ENGINE_MAJOR_VERSION > 4 /*&& ENGINE_MINOR_VERSION > 0*/
		TOptional<FAssetPackageData> PackageDataOpt = AssetRegistryModule.Get().GetAssetPackageDataCopy(*TargetLongPackageName);
		if(PackageDataOpt.IsSet())
		{
			const FAssetPackageData* PackageData = &PackageDataOpt.GetValue();
#else
			const FAssetPackageData* PackageData = AssetRegistryModule.Get().GetAssetPackageData(*TargetLongPackageName);
#endif
			AssetPackageData = const_cast<FAssetPackageData*>(PackageData);
			if (AssetPackageData != nullptr)
			{
				return AssetPackageData;
			}
#if ENGINE_MAJOR_VERSION > 4 /*&& ENGINE_MINOR_VERSION > 0*/
		}
#endif
	}

	return NULL;
}
#pragma warning(pop)

bool UFlibAssetManageHelper::ConvLongPackageNameToCookedPath(
	const FString& InProjectAbsDir,
	const FString& InPlatformName,
	const FString& InLongPackageName,
	TArray<FString>& OutCookedAssetPath,
	TArray<FString>& OutCookedAssetRelativePath,
	const FString& OverrideCookedDir
	)
{
	if (!FPaths::DirectoryExists(InProjectAbsDir)/* || !IsValidPlatform(InPlatformName)*/)
		return false;

	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	FString CookedRootDir = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"), InPlatformName);
	if(!OverrideCookedDir.IsEmpty() && FPaths::DirectoryExists(OverrideCookedDir))
	{
		CookedRootDir = FPaths::Combine(OverrideCookedDir,InPlatformName);
	}
	FString ProjectName = FApp::GetProjectName();
	// FString AssetPackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(InLongPackageName);
	FString AssetAbsPath = UFlibAssetManageHelper::LongPackageNameToFilename(InLongPackageName);

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
	const FString& OverrideCookedDir,
	const FString& InPlatformName, 
	const FAssetDependenciesInfo& InAssetDependencies, 
	//const TArray<FString> &InCookParams, 
	TArray<FString>& OutCookCommand, 
	TFunction<void(const TArray<FString>&,const TArray<FString>&, const FString&, const FString&)> InReceivePakCommand,
	TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset
)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::MakePakCommandFromAssetDependencies",FColor::Red);
	if (!FPaths::DirectoryExists(InProjectDir) /*|| !UFlibAssetManageHelper::IsValidPlatform(InPlatformName)*/)
		return false;
	OutCookCommand.Empty();
	// TArray<FString> resault;
	TArray<FString> Keys;
	InAssetDependencies.AssetsDependenciesMap.GetKeys(Keys);
	FCriticalSection	LocalSynchronizationObject;
	ParallelFor(Keys.Num(),[&](int32 index)
	{
		const FString& Key = Keys[index];
		if (Key.Equals(TEXT("Script")))
			return;
		TArray<FString> AssetList;
		InAssetDependencies.AssetsDependenciesMap.Find(Key)->AssetDependencyDetails.GetKeys(AssetList);
		for (const auto& AssetLongPackageName : AssetList)
		{
			TArray<FString> FinalCookedCommand;
			if (UFlibAssetManageHelper::MakePakCommandFromLongPackageName(InProjectDir, OverrideCookedDir,InPlatformName, AssetLongPackageName, /*InCookParams, */FinalCookedCommand,InReceivePakCommand,IsIoStoreAsset))
			{
				FScopeLock Lock(&LocalSynchronizationObject);
				OutCookCommand.Append(FinalCookedCommand);
			}
		}
	},true);
	return true;
}


bool UFlibAssetManageHelper::MakePakCommandFromLongPackageName(
	const FString& InProjectDir,
	const FString& OverrideCookedDir,
	const FString& InPlatformName, 
	const FString& InAssetLongPackageName, 
	//const TArray<FString> &InCookParams, 
	TArray<FString>& OutCookCommand,
	TFunction<void(const TArray<FString>&,const TArray<FString>&, const FString&, const FString&)> InReceivePakCommand,
	TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset
)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::MakePakCommandFromLongPackageName",FColor::Red);
	OutCookCommand.Empty();
	bool bStatus = false;
	TArray<FString> CookedAssetAbsPath;
	TArray<FString> CookedAssetRelativePath;
	TArray<FString> FinalCookedPakCommand;
	TArray<FString> FinalCookedIoStoreCommand;
	if (UFlibAssetManageHelper::ConvLongPackageNameToCookedPath(InProjectDir, InPlatformName, InAssetLongPackageName, CookedAssetAbsPath, CookedAssetRelativePath,OverrideCookedDir))
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::CombineCookedAssetCommand",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAllEnabledModuleName",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::ModuleIsEnabled",FColor::Red);
	TMap<FString,FString> AllEnabledModules;
	UFlibAssetManageHelper::GetAllEnabledModuleName(AllEnabledModules);
	TArray<FString> AllEnableModuleNames;
	AllEnabledModules.GetKeys(AllEnableModuleNames);
	return AllEnabledModules.Contains(InModuleName);
}

bool UFlibAssetManageHelper::GetModuleNameByRelativePath(const FString& InRelativePath, FString& OutModuleName)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetModuleNameByRelativePath",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetBelongModuleName",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::FindFilesRecursive",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::ParserAssetDependenciesInfoNumber",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::ParserModuleAssetsNumMap",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::GetAssetDataInPaths",FColor::Red);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::LongPackageNameToPackagePath",FColor::Red);
	if(InLongPackageName.Contains(TEXT(".")))
	{
		// UE_LOG(LogHotPatcher,Warning,TEXT("LongPackageNameToPackagePath %s alway is PackagePath!"),*InLongPackageName);
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
	SCOPED_NAMED_EVENT_TEXT("UFlibAssetManageHelper::PackagePathToLongPackageName",FColor::Red);
	FSoftObjectPath ObjectPath = PackagePath;
	return ObjectPath.GetLongPackageName();
}

void UFlibAssetManageHelper::ExcludeContentForAssetDependenciesDetail(FAssetDependenciesInfo& AssetDependencies,const TArray<FString>& ExcludeRules,EHotPatcherMatchModEx matchMod)
{
	SCOPED_NAMED_EVENT_TEXT("ExcludeContentForAssetDependenciesDetail",FColor::Red);
	auto ExcludeEditorContent = [&ExcludeRules,matchMod](TMap<FString,FAssetDependenciesDetail>& AssetCategorys)
	{
		TArray<FString> Keys;
		AssetCategorys.GetKeys(Keys);
		
		for(const auto& Key:Keys)
		{
			FAssetDependenciesDetail&  ModuleAssetList = *AssetCategorys.Find(Key);
			TArray<FString> AssetKeys;
			
			ModuleAssetList.AssetDependencyDetails.GetKeys(AssetKeys);
			FCriticalSection	SynchronizationObject;
			
			// for(const auto& AssetKey:AssetKeys)
			ParallelFor(AssetKeys.Num(),[&](int32 index)
			{
				auto& AssetKey = AssetKeys[index];
				bool customStartWith = false;
				FString MatchRule;
				for(const auto& Rule:ExcludeRules)
				{
					if((matchMod == EHotPatcherMatchModEx::StartWith && AssetKeys[index].StartsWith(Rule)) ||
						(matchMod == EHotPatcherMatchModEx::Equal && AssetKeys[index].Equals(Rule))
					)
					{
						MatchRule = Rule;
						customStartWith = true;
						break;
					}
				}
				if( customStartWith )
				{
					// UE_LOG(LogHotPatcher,Display,TEXT("remove %s in AssetDependencies(match exclude rule %s)"),*AssetKey,*MatchRule);
					FScopeLock Lock(&SynchronizationObject);
					ModuleAssetList.AssetDependencyDetails.Remove(AssetKey);
				}
			},GForceSingleThread);
		}
	};
	ExcludeEditorContent(AssetDependencies.AssetsDependenciesMap);
}

TArray<FString> UFlibAssetManageHelper::DirectoriesToStrings(const TArray<FDirectoryPath>& DirectoryPaths)
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

TSet<FName> UFlibAssetManageHelper::GetClassesNames(const TArray<UClass*> CLasses)
{
	TSet<FName> ForceSkipTypes;
	for(const auto& Classes:CLasses)
	{
		ForceSkipTypes.Add(*Classes->GetName());
	}
	return ForceSkipTypes;
}

FString UFlibAssetManageHelper::NormalizeContentDir(const FString& Dir)
{
	FString result = Dir;

#if ENGINE_MAJOR_VERSION > 4
	if(result.StartsWith(TEXT("/All/"),ESearchCase::IgnoreCase))
	{
		result.RemoveFromStart(TEXT("/All"),ESearchCase::IgnoreCase);
	}
#endif
	if(!Dir.EndsWith(TEXT("/")))
	{
		result = FString::Printf(TEXT("%s/"),*result);
	}

	return result;
}

TArray<FString> UFlibAssetManageHelper::NormalizeContentDirs(const TArray<FString>& Dirs)
{
	TArray<FString> NormalizedDirs;
	for(const auto& Dir:Dirs)
	{
		NormalizedDirs.AddUnique(UFlibAssetManageHelper::NormalizeContentDir(Dir));
	}
	return NormalizedDirs;
}


FStreamableManager& UFlibAssetManageHelper::GetStreamableManager()
{
	return UAssetManager::GetStreamableManager();
}

TSharedPtr<FStreamableHandle> UFlibAssetManageHelper::LoadObjectAsync(FSoftObjectPath ObjectPath,TFunction<void(FSoftObjectPath)> Callback,uint32 Priority)
{
	TSharedPtr<FStreamableHandle> Handle = GetStreamableManager().RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateLambda([=]()
	{
		if(Callback)
		{
			Callback(ObjectPath);
		}
	}), Priority, false);
	return Handle;
}

void UFlibAssetManageHelper::LoadPackageAsync(FSoftObjectPath ObjectPath,TFunction<void(FSoftObjectPath,bool)> Callback,uint32 Priority)
{
	::LoadPackageAsync(ObjectPath.GetLongPackageName(), nullptr,nullptr,FLoadPackageAsyncDelegate::CreateLambda([=](const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result)
	{
		if(Callback && Result == EAsyncLoadingResult::Succeeded)
		{
			Package->AddToRoot();
			Callback(UFlibAssetManageHelper::LongPackageNameToPackagePath(Package->GetPathName()),Result == EAsyncLoadingResult::Succeeded);
		}
	}));
}

UPackage* UFlibAssetManageHelper::LoadPackage(UPackage* InOuter, const TCHAR* InLongPackageName, uint32 LoadFlags,
	FArchive* InReaderOverride)
{
#if ENGINE_MINOR_VERSION < 26
	FScopedNamedEvent CookPackageEvent(FColor::Red,*FString::Printf(TEXT("LoadPackage %s"),InLongPackageName));
#endif
	UE_LOG(LogHotPatcher,Verbose,TEXT("Load %s,outer %s"),InLongPackageName,InOuter ? *InOuter->GetFullName():TEXT("null"));
	return ::LoadPackage(InOuter,InLongPackageName,LoadFlags,InReaderOverride);
}

UPackage* UFlibAssetManageHelper::GetPackage(FName PackageName)
{
#if ENGINE_MINOR_VERSION < 26
	FScopedNamedEvent CookPackageEvent(FColor::Red,*FString::Printf(TEXT("GetPackage %s"),*PackageName.ToString()));
#endif
	if (PackageName == NAME_None)
	{
		return NULL;
	}

	UPackage* Package = FindPackage(NULL, *PackageName.ToString());
	if (Package)
	{
		Package->FullyLoad();
	}
	else
	{
		Package = UFlibAssetManageHelper::LoadPackage(NULL, *PackageName.ToString(), LOAD_None);
	}

	return Package;
}

// TArray<UPackage*> UFlibAssetManageHelper::GetPackagesByClass(TArray<UPackage*>& Packages, UClass* Class, bool RemoveFromSrc)
// {
// 	// TArray<UPackage*> result;
// 	// for(int32 PkgIndex = Packages.Num() - 1 ;PkgIndex >= 0;--PkgIndex)
// 	// {
// 	// 	if(Packages[PkgIndex]->IsA(Class))
// 	// 	{
// 	// 		result.Add(Packages[PkgIndex]);
// 	// 		if(RemoveFromSrc)
// 	// 		{
// 	// 			Packages.RemoveAtSwap(PkgIndex,1,false);
// 	// 		}
// 	// 		
// 	// 	}
// 	// }
// 	// return result;
// 	return THotPatcherTemplateHelper::GetArrayBySrcWithCondition<UPackage*>(Packages,[&](UPackage* Package)->bool
// 	{
// 		return Package->IsA(Class);
// 	},true);
// };

TArray<UPackage*> UFlibAssetManageHelper::LoadPackagesForCooking(const TArray<FSoftObjectPath>& SoftObjectPaths, bool bStorageConcurrent)
{
	SCOPED_NAMED_EVENT_TEXT("LoadPackagesForCooking",FColor::Red);
	TArray<UPackage*> AllPackages;
	GIsCookerLoadingPackage = true;
	for(const auto& Asset:SoftObjectPaths)
	{
		UPackage* Package = UFlibAssetManageHelper::GetPackage(*Asset.GetLongPackageName());
		if(!Package)
		{
			UE_LOG(LogHotPatcher,Warning,TEXT("LodPackage %s is null!"),*Asset.GetLongPackageName());
			continue;
		}
		AllPackages.AddUnique(Package);
		
	}
	
	for(auto Package:AllPackages)
	{
		if(!bStorageConcurrent && Package->IsFullyLoaded())
		{
			UMetaData* MetaData = Package->GetMetaData();
			if(MetaData)
			{
				MetaData->RemoveMetaDataOutsidePackage();
			}
		}
		// Precache the metadata so we don't risk rehashing the map in the parallelfor below
		if(bStorageConcurrent)
		{
			if(!Package->IsFullyLoaded())
			{
				Package->FullyLoad();
			}
			Package->GetMetaData();
		}
	}
	GIsCookerLoadingPackage = false;
	return AllPackages;
}

bool UFlibAssetManageHelper::MatchIgnoreTypes(const FString& LongPackageName, TSet<FName> IgnoreTypes, FString& MatchTypeStr)
{
	bool bIgnoreType = false;
	FAssetData AssetData;
	if(UFlibAssetManageHelper::GetAssetsDataByPackageName(LongPackageName,AssetData))
	{
		SCOPED_NAMED_EVENT_TEXT("is ignore types",FColor::Red);
		MatchTypeStr = UFlibAssetManageHelper::GetAssetDataClasses(AssetData).ToString();
		FName ClassName = UFlibAssetManageHelper::GetAssetDataClasses(AssetData);
		bIgnoreType = IgnoreTypes.Contains(ClassName);

		if(!bIgnoreType)
		{
#if (ENGINE_MAJOR_VERSION > 4) || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25)
			bIgnoreType = AssetData.HasAnyPackageFlags(PKG_EditorOnly);
#else
			// bIgnoreType = (AssetData.PackageFlags & PKG_EditorOnly) != 0;
			
#endif
			if(bIgnoreType)
			{
				MatchTypeStr = TEXT("Has PKG_EditorOnly Flag");
			}
		}
	}
	return bIgnoreType;
}

bool UFlibAssetManageHelper::MatchIgnoreFilters(const FString& LongPackageName, const TArray<FString>& IgnoreDirs,
	FString& MatchDir)
{
	for(const auto& IgnoreFilter:IgnoreDirs)
	{
		bool bWithInSkipDir = LongPackageName.StartsWith(IgnoreFilter);
		bool bMatchSkipWildcard = IgnoreFilter.Contains(TEXT("*")) ? LongPackageName.MatchesWildcard(IgnoreFilter,ESearchCase::CaseSensitive) : false;
		if( bWithInSkipDir || bMatchSkipWildcard)
		{
			MatchDir = IgnoreFilter;
			return true;
		}
	}
	return false;
}


bool UFlibAssetManageHelper::ContainsRedirector(const FName& PackageName, TMap<FName, FName>& RedirectedPaths)
{
	bool bFoundRedirector = false;
	TArray<FAssetData> Assets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry* AssetRegistry = &AssetRegistryModule.Get();
	ensure(AssetRegistry->GetAssetsByPackageName(PackageName, Assets, true));

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
				FAssetData DestinationData = AssetRegistry->GetAssetByObjectPath(RedirectedPath, true);
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
							DestinationData = AssetRegistry->GetAssetByObjectPath(RedirectedPath, true);
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

				// if (!bDestinationValid)
				// {
				// 	// we can;t call GetCachedStandardPackageFileFName with None
				// 	if (RedirectedPath != NAME_None)
				// 	{
				// 		FName StandardPackageName = PackageNameCache->GetCachedStandardPackageFileFName(FName(*FPackageName::ObjectPathToPackageName(RedirectedPathString)));
				// 		if (StandardPackageName != NAME_None)
				// 		{
				// 			bDestinationValid = true;
				// 		}
				// 	}
				// }

				if (bDestinationValid)
				{
					RedirectedPaths.Add(UFlibAssetManageHelper::GetObjectPathByAssetData(Asset), RedirectedPath);
				}
				else
				{
					RedirectedPaths.Add(UFlibAssetManageHelper::GetObjectPathByAssetData(Asset), NAME_None);
					UE_LOG(LogHotPatcher, Log, TEXT("Found redirector in package %s pointing to deleted object %s"), *PackageName.ToString(), *RedirectedPathString);
				}

				bFoundRedirector = true;
			}
		}
	}
	return bFoundRedirector;
}


TArray<UObject*> UFlibAssetManageHelper::FindClassObjectInPackage(UPackage* Package,UClass* FindClass)
{
	TArray<UObject*> ResultObjects;
	TArray<UObject*> ObjectsInPackage;
	GetObjectsWithOuter(Package, ObjectsInPackage, false);
	for ( auto ObjIt = ObjectsInPackage.CreateConstIterator(); ObjIt; ++ObjIt )
	{
		if((*ObjIt)->GetClass()->IsChildOf(FindClass))
		{
			ResultObjects.Add(*ObjIt);
		}
	}
	return ResultObjects;
}
bool UFlibAssetManageHelper::HasClassObjInPackage(UPackage* Package,UClass* FindClass)
{
	return !!FindClassObjectInPackage(Package,FindClass).Num();
}

TArray<FAssetDetail> UFlibAssetManageHelper::GetAssetDetailsByClass(TArray<FAssetDetail>& AllAssetDetails,
	UClass* Class, bool RemoveFromSrc)
{
	SCOPED_NAMED_EVENT_TEXT("GetAssetDetailsByClass",FColor::Red);
	return THotPatcherTemplateHelper::GetArrayBySrcWithCondition<FAssetDetail>(AllAssetDetails,[&](FAssetDetail AssetDetail)->bool
				{
					return AssetDetail.AssetType.IsEqual(*Class->GetName());
				},RemoveFromSrc);
}

TArray<FSoftObjectPath> UFlibAssetManageHelper::GetAssetPathsByClass(TArray<FAssetDetail>& AllAssetDetails,
	UClass* Class, bool RemoveFromSrc)
{
	TArray<FSoftObjectPath> ObjectPaths;
	for(const auto& AssetDetail:UFlibAssetManageHelper::GetAssetDetailsByClass(AllAssetDetails,Class,RemoveFromSrc))
	{
		ObjectPaths.Emplace(AssetDetail.PackagePath.ToString());
	}
	return ObjectPaths;
}

void UFlibAssetManageHelper::ReplaceReditector(TArray<FAssetDetail>& SrcAssets)
{
	SCOPED_NAMED_EVENT_TEXT("ReplaceReditector",FColor::Red);
	for(auto& AssetDetail:SrcAssets)
	{
		FName SrcPackagePath = AssetDetail.PackagePath;
		
		if(IsRedirector(AssetDetail,AssetDetail))
		{
			UE_LOG(LogHotPatcher,Warning,TEXT("%s is an redirector(to %s)!"),*SrcPackagePath.ToString(),*AssetDetail.PackagePath.ToString())
		}
	}
}

void UFlibAssetManageHelper::RemoveInvalidAssets(TArray<FAssetDetail>& SrcAssets)
{
	SCOPED_NAMED_EVENT_TEXT("RemoveInvalidAssets",FColor::Red);
	for(size_t index = 0;index< SrcAssets.Num();)
	{
		FString PackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(SrcAssets[index].PackagePath.ToString());
		if(FPackageName::DoesPackageExist(PackageName))
		{
			++index;
			continue;
		}
		else
		{
			UE_LOG(LogHotPatcher,Warning,TEXT("%s is not a valid package."),*PackageName);
			SrcAssets.RemoveAt(index);
		}
	}
}
#include "Misc/EngineVersionComparison.h"
FName UFlibAssetManageHelper::GetAssetDataClasses(const FAssetData& Data)
{
	if(Data.IsValid())
	{
#if !UE_VERSION_OLDER_THAN(5,1,0)
		return Data.AssetClassPath.GetAssetName();
#else
		return Data.AssetClass;
#endif
	}
	return NAME_None;
}

FName UFlibAssetManageHelper::GetObjectPathByAssetData(const FAssetData& Data)
{
	if(Data.IsValid())
	{
#if !UE_VERSION_OLDER_THAN(5,1,0)
		return *UFlibAssetManageHelper::LongPackageNameToPackagePath(Data.PackageName.ToString());
#else
		return Data.ObjectPath;
#endif
	}
	return NAME_None;
}

void UFlibAssetManageHelper::UpdateAssetRegistryData(const FString& PackageName)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	FString PackageFilename;
	if (FPackageName::FindPackageFileWithoutExtension(FPackageName::LongPackageNameToFilename(PackageName), PackageFilename))
	{
		AssetRegistry.ScanModifiedAssetFiles(TArray<FString>{PackageFilename});
	}
}
TArray<FString> UFlibAssetManageHelper::GetPackgeFiles(const FString& LongPackageName,const FString& Extension)
{
	SCOPED_NAMED_EVENT_TEXT("GetPackgeFiles",FColor::Red);
	TArray<FString> Files;
			
	FString FilePath = FPackageName::LongPackageNameToFilename(LongPackageName) + Extension;

	if(FPaths::FileExists(FilePath))
	{
		FPaths::MakeStandardFilename(FilePath);
		Files.Add(FilePath);
	}
	return Files;
};

// PRAGMA_ENABLE_DEPRECATION_WARNINGS
