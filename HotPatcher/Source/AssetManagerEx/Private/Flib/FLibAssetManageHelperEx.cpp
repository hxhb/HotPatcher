// Fill out your copyright notice in the Description page of Project Settings.


#include "FLibAssetManageHelperEx.h"
#include "AssetManager/FAssetDependenciesInfo.h"
#include "AssetManager/FAssetDependenciesDetail.h"
#include "AssetManager/FFileArrayDirectoryVisitor.hpp"

#include "ARFilter.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Json.h"
#include "Templates/SharedPointer.h"
#include "IPluginManager.h"
#include "Engine/AssetManager.h"
#include "AssetData.h"
#ifdef __DEVELOPER_MODE__
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#endif

FString UFLibAssetManageHelperEx::ConvVirtualToAbsPath(const FString& InPackagePath)
{
	FString ResultAbsPath;

	FString AssetAbsNotPostfix = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(UFLibAssetManageHelperEx::GetLongPackageNameFromPackagePath(InPackagePath)));
	FString AssetName = UFLibAssetManageHelperEx::GetAssetNameFromPackagePath(InPackagePath);
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


bool UFLibAssetManageHelperEx::ConvAbsToVirtualPath(const FString& InAbsPath, FString& OutPackagePath)
{
	bool runState = false;
	FString LongPackageName;
	runState = FPackageName::TryConvertFilenameToLongPackageName(InAbsPath, LongPackageName);

	if (runState)
	{
		FString PackagePath;
		if (UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(LongPackageName, PackagePath))
		{
			OutPackagePath = PackagePath;
			runState = runState && true;
		}
	}
	
	return runState;
}

FString UFLibAssetManageHelperEx::GetLongPackageNameFromPackagePath(const FString& InPackagePath)
{
	int32 FoundIndex;
	if (InPackagePath.FindLastChar('.', FoundIndex))
	{
		FStringAssetReference InAssetRef = InPackagePath;
		return InAssetRef.GetLongPackageName();
	}
	else
	{
		return InPackagePath;
	}
}

FString UFLibAssetManageHelperEx::GetAssetNameFromPackagePath(const FString& InPackagePath)
{
	FStringAssetReference InAssetRef = InPackagePath;
	return InAssetRef.GetAssetName();
}


bool UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(const FString& InLongPackageName, FString& OutPackagePath)
{
	OutPackagePath.Empty();
	bool runState = false;
	if (FPackageName::DoesPackageExist(InLongPackageName))
	{
		FString AssetName;
		{
			int32 FoundIndex;
			InLongPackageName.FindLastChar('/', FoundIndex);
			if (FoundIndex != INDEX_NONE)
			{
				AssetName = UKismetStringLibrary::GetSubstring(InLongPackageName, FoundIndex + 1, InLongPackageName.Len() - FoundIndex);
			}
		}
		OutPackagePath = InLongPackageName + TEXT(".") + AssetName;
		runState = true;
	}
	return runState;
}

bool UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(const FString& InPackagePath, FString& OutLongPackageName)
{
	bool brunstatus=false;
	FSoftObjectPath PackageSoftRef{ InPackagePath };
	if (PackageSoftRef.IsValid())
	{
		OutLongPackageName = PackageSoftRef.GetLongPackageName();
		brunstatus = true;
	}

	return brunstatus;
}

bool UFLibAssetManageHelperEx::GetAssetPackageGUID(const FString& InPackagePath, FString& OutGUID)
{
	bool bResult = false;
	if (InPackagePath.IsEmpty())
		return false;

	const FAssetPackageData* AssetPackageData = UFLibAssetManageHelperEx::GetPackageDataByPackagePath(InPackagePath);
	if (AssetPackageData != NULL)
	{
		const FGuid& AssetGuid = AssetPackageData->PackageGuid;
		OutGUID = AssetGuid.ToString();
		bResult = true;
	}
	return bResult;
}


FAssetDependenciesInfo UFLibAssetManageHelperEx::CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B)
{
	FAssetDependenciesInfo resault;

	auto CombineLambda = [&resault](const FAssetDependenciesInfo& InDependencies)
	{
		TArray<FString> Keys;
		InDependencies.mDependencies.GetKeys(Keys);
		for (const auto& Key : Keys)
		{
			if (!resault.mDependencies.Contains(Key))
			{
				resault.mDependencies.Add(Key, *InDependencies.mDependencies.Find(Key));
			}
			else
			{

				{
					TMap<FString,FAssetDetail>& ExistingAssetDetails = resault.mDependencies.Find(Key)->mDependAssetDetails;
					TArray<FString> ExistingAssetList;
					ExistingAssetDetails.GetKeys(ExistingAssetList);

					const TMap<FString,FAssetDetail>& PaddingAssetDetails = InDependencies.mDependencies.Find(Key)->mDependAssetDetails;
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

void UFLibAssetManageHelperEx::GetAssetDependencies(const FString& InLongPackageName, FAssetDependenciesInfo& OutDependices)
{
	if (InLongPackageName.IsEmpty())
		return;

	FStringAssetReference AssetRef = FStringAssetReference(InLongPackageName);
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
				UE_LOG(LogTemp, Error, TEXT("Faild to Parser AssetData of %s, please check."), *InLongPackageName);
				return;
			}
			if (AssetDataList.Num() > 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Got mulitple AssetData of %s,please check."), *InLongPackageName);
			}
		}
		UFLibAssetManageHelperEx::GatherAssetDependicesInfoRecursively(AssetRegistryModule, InLongPackageName, OutDependices);
	}
}

void UFLibAssetManageHelperEx::GetAssetListDependencies(const TArray<FString>& InLongPackageNameList, FAssetDependenciesInfo& OutDependices)
{
	FAssetDependenciesInfo result;

	for (const auto& LongPackageItem : InLongPackageNameList)
	{
		FAssetDependenciesInfo CurrentDependency;
		UFLibAssetManageHelperEx::GetAssetDependencies(LongPackageItem, CurrentDependency);
		result = UFLibAssetManageHelperEx::CombineAssetDependencies(result, CurrentDependency);
	}
	OutDependices = result;
}

void UFLibAssetManageHelperEx::GetAssetDependenciesForAssetDetail(const FAssetDetail& InAssetDetail, FAssetDependenciesInfo& OutDependices)
{
	FString AssetPackagePath = InAssetDetail.mPackagePath;
	FSoftObjectPath AssetObjectSoftRef{ AssetPackagePath };

	GetAssetDependencies(AssetObjectSoftRef.GetLongPackageName(), OutDependices);
}

void UFLibAssetManageHelperEx::GetAssetListDependenciesForAssetDetail(const TArray<FAssetDetail>& InAssetsDetailList, FAssetDependenciesInfo& OutDependices)
{
	FAssetDependenciesInfo result;
	for (const auto& AssetDetail : InAssetsDetailList)
	{
		FAssetDependenciesInfo CurrentAsserInfo;
		UFLibAssetManageHelperEx::GetAssetDependenciesForAssetDetail(AssetDetail, CurrentAsserInfo);
		result = UFLibAssetManageHelperEx::CombineAssetDependencies(result, CurrentAsserInfo);
	}
	OutDependices = result;
}

void UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(const FAssetDependenciesInfo& InAssetDependencies,TArray<FAssetDetail>& OutAssetDetails)
{
		OutAssetDetails.Empty();
		TArray<FString> Keys;
		InAssetDependencies.mDependencies.GetKeys(Keys);

		for (const auto& Key : Keys)
		{
			TMap<FString, FAssetDetail> ModuleAssetDetails = InAssetDependencies.mDependencies.Find(Key)->mDependAssetDetails;

			TArray<FString> ModuleAssetKeys;
			ModuleAssetDetails.GetKeys(ModuleAssetKeys);

			for (const auto& ModuleAssetKey : ModuleAssetKeys)
			{
				OutAssetDetails.Add(*ModuleAssetDetails.Find(ModuleAssetKey));
			}
		}
}

void UFLibAssetManageHelperEx::GatherAssetDependicesInfoRecursively(
	FAssetRegistryModule& InAssetRegistryModule,
	const FString& InTargetLongPackageName,
	FAssetDependenciesInfo& OutDependencies
)
{
	TArray<FName> local_Dependencies;
	bool bGetDependenciesSuccess = InAssetRegistryModule.Get().GetDependencies(FName(*InTargetLongPackageName), local_Dependencies, EAssetRegistryDependencyType::Packages);
	if (bGetDependenciesSuccess)
	{
		for (auto &DependItem : local_Dependencies)
		{
			FString LongDependentPackageName = DependItem.ToString();
			FString BelongModuleName = UFLibAssetManageHelperEx::GetAssetBelongModuleName(LongDependentPackageName);

			// add a new asset to module category
			auto AddNewAssetItemLambda = [&InAssetRegistryModule, &OutDependencies](
				FAssetDependenciesDetail& InModuleAssetDependDetail,
				const FString& InAssetPackageName
				)
			{
				if (!InModuleAssetDependDetail.mDependAssetDetails.Contains(InAssetPackageName))
				{
				 	// FAssetData* AssetPackageData = InAssetRegistryModule.Get().GetAssetPackageData(*InAssetPackageName);
					UAssetManager& AssetManager = UAssetManager::Get();
					if (AssetManager.IsValid())
					{
						FString PackagePath;
						UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(InAssetPackageName,PackagePath);
						FAssetData OutAssetData;
						if (AssetManager.GetAssetDataForPath(FSoftObjectPath{ PackagePath }, OutAssetData))
						{
							FAssetDetail AssetDetail;
							AssetDetail.mPackagePath = PackagePath;
							AssetDetail.mAssetType = OutAssetData.AssetClass.ToString();
							UFLibAssetManageHelperEx::GetAssetPackageGUID(PackagePath, AssetDetail.mGuid);
							InModuleAssetDependDetail.mDependAssetDetails.Add(InAssetPackageName,AssetDetail);
						}
					}
					
					GatherAssetDependicesInfoRecursively(InAssetRegistryModule, InAssetPackageName, OutDependencies);
				}
			};

			if (OutDependencies.mDependencies.Contains(BelongModuleName))
			{
				// UE_LOG(LogTemp, Log, TEXT("Belong Module is %s,Asset is %s"), *BelongModuleName, *LongDependentPackageName);
				FAssetDependenciesDetail* ModuleCategory = OutDependencies.mDependencies.Find(BelongModuleName);
				AddNewAssetItemLambda(*ModuleCategory, LongDependentPackageName);
			}
			else
			{
				// UE_LOG(LogTemp, Log, TEXT("New Belong Module is %s,Asset is %s"), *BelongModuleName,*LongDependentPackageName);
				FAssetDependenciesDetail& NewModuleCategory = OutDependencies.mDependencies.Add(BelongModuleName, FAssetDependenciesDetail{});
				NewModuleCategory.mModuleCategory = BelongModuleName;
				AddNewAssetItemLambda(NewModuleCategory, LongDependentPackageName);
			}
		}
	}
}


bool UFLibAssetManageHelperEx::GetModuleAssetsList(const FString& InModuleName, const TArray<FString>& InExFilterPackagePaths, TArray<FAssetDetail>& OutAssetList)
{
	TArray<FString> AllEnableModule;
	UFLibAssetManageHelperEx::GetAllEnabledModuleName(AllEnableModule);

	if (!AllEnableModule.Contains(InModuleName))
		return false;
	TArray<FString> AllFilterPackageNames;
	AllFilterPackageNames.AddUnique(TEXT("/") + InModuleName);
	for (const auto& ExFilterPackageName : InExFilterPackagePaths)
	{
		AllFilterPackageNames.AddUnique(ExFilterPackageName);
	}
	return UFLibAssetManageHelperEx::GetAssetsList(AllFilterPackageNames, OutAssetList);
}

bool UFLibAssetManageHelperEx::GetAssetsList(const TArray<FString>& InFilterPackagePaths, TArray<FAssetDetail>& OutAssetList)
{
	TArray<FAssetData> AllAssetData;
	if (UFLibAssetManageHelperEx::GetAssetsData(InFilterPackagePaths, AllAssetData))
	{
		for (const auto& AssetDataIndex : AllAssetData)
		{
			FAssetDetail AssetDetail;
			UFLibAssetManageHelperEx::ConvFAssetDataToFAssetDetail(AssetDataIndex, AssetDetail);
			OutAssetList.Add(AssetDetail);
		}
		return true;
	}
	return false;
}


bool UFLibAssetManageHelperEx::GetSpecifyAssetData(const FString& InLongPackageName, TArray<FAssetData>& OutAssetData, bool InIncludeOnlyOnDiskAssets)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	return AssetRegistryModule.Get().GetAssetsByPackageName(*InLongPackageName, OutAssetData, InIncludeOnlyOnDiskAssets);
}

bool UFLibAssetManageHelperEx::GetAssetsData(const TArray<FString>& InFilterPackagePaths, TArray<FAssetData>& OutAssetData)
{
	OutAssetData.Reset();

	FARFilter Filter;
	Filter.bIncludeOnlyOnDiskAssets = true;
	Filter.bRecursivePaths = true;
	for(const auto& FilterPackageName: InFilterPackagePaths)
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

bool UFLibAssetManageHelperEx::GetSingleAssetsData(const FString& InPackagePath, FAssetData& OutAssetData)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	return AssetManager.GetAssetDataForPath(FSoftObjectPath{ InPackagePath }, OutAssetData);
}

bool UFLibAssetManageHelperEx::GetClassStringFromFAssetData(const FAssetData& InAssetData, FString& OutAssetType)
{
	bool bRunState = false;
	if (InAssetData.IsValid())
	{
		OutAssetType = InAssetData.AssetClass.ToString();
		bRunState = true;
	}
	return bRunState;
}


bool UFLibAssetManageHelperEx::ConvFAssetDataToFAssetDetail(const FAssetData& InAssetData, FAssetDetail& OutAssetDetail)
{
	FAssetDetail AssetDetail;
	AssetDetail.mAssetType = InAssetData.AssetClass.ToString();
	UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(InAssetData.PackageName.ToString(), AssetDetail.mPackagePath);

	UFLibAssetManageHelperEx::GetAssetPackageGUID(InAssetData.PackageName.ToString(), AssetDetail.mGuid);

	OutAssetDetail = AssetDetail;
	return true;
}

bool UFLibAssetManageHelperEx::GetSpecifyAssetDetail(const FString& InLongPackageName, FAssetDetail& OutAssetDetail)
{

	bool bRunStatus = false;
	TArray<FAssetData> AssetData;
	if (UFLibAssetManageHelperEx::GetSpecifyAssetData(InLongPackageName, AssetData, true))
	{
		if (AssetData.Num() > 0)
		{
			UFLibAssetManageHelperEx::ConvFAssetDataToFAssetDetail(AssetData[0], OutAssetDetail);
			bRunStatus = true;
		}
	}
	return bRunStatus;
}

void UFLibAssetManageHelperEx::FilterNoRefAssets(const TArray<FAssetDetail>& InAssetsDetail, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail)
{
	OutHasRefAssetsDetail.Reset();
	OutDontHasRefAssetsDetail.Reset();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (const auto& AssetDetail : InAssetsDetail)
	{
		FSoftObjectPath CurrentObjectSoftRef{ AssetDetail.mPackagePath };
		FAssetIdentifier CurrentAssetId{ *CurrentObjectSoftRef.GetLongPackageName() };
		
		// ignore scan Map Asset reference
		{
			FAssetData CurrentAssetData;
			if (UFLibAssetManageHelperEx::GetSingleAssetsData(AssetDetail.mPackagePath, CurrentAssetData))
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

void UFLibAssetManageHelperEx::FilterNoRefAssetsWithIgnoreFilter(const TArray<FAssetDetail>& InAssetsDetail, const TArray<FString>& InIgnoreFilters, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail)
{
	OutHasRefAssetsDetail.Reset();
	OutDontHasRefAssetsDetail.Reset();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	for (const auto& AssetDetail : InAssetsDetail)
	{
		FSoftObjectPath CurrentObjectSoftRef{ AssetDetail.mPackagePath };
		FAssetIdentifier CurrentAssetId{ *CurrentObjectSoftRef.GetLongPackageName() };

		// ignore scan Map Asset reference
		{
			FAssetData CurrentAssetData;
			if (UFLibAssetManageHelperEx::GetSingleAssetsData(AssetDetail.mPackagePath, CurrentAssetData))
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
bool UFLibAssetManageHelperEx::CombineAssetsDetailAsFAssetDepenInfo(const TArray<FAssetDetail>& InAssetsDetailList, FAssetDependenciesInfo& OutAssetInfo)
{
	FAssetDependenciesInfo result;

	for (const auto& AssetDetail : InAssetsDetailList)
	{
		FString CurrAssetModuleName = UFLibAssetManageHelperEx::GetAssetBelongModuleName(AssetDetail.mPackagePath);
		FSoftObjectPath CurrAssetObjectPath(AssetDetail.mPackagePath);
		FString CurrAssetLongPackageName = CurrAssetObjectPath.GetLongPackageName();
		if (!result.mDependencies.Contains(CurrAssetModuleName))
		{
			FAssetDependenciesDetail AssetDependenciesDetail{ CurrAssetModuleName,TMap<FString,FAssetDetail>{ {CurrAssetLongPackageName,AssetDetail} } };
			result.mDependencies.Add(CurrAssetModuleName, AssetDependenciesDetail);
		}
		else
		{
			FAssetDependenciesDetail& CurrentCategory = *result.mDependencies.Find(CurrAssetModuleName);
			
			if (!result.mDependencies.Contains(CurrAssetLongPackageName))
			{
				CurrentCategory.mDependAssetDetails.Add(CurrAssetLongPackageName,AssetDetail);
			}
		}
	
	}
	OutAssetInfo = result;

	return true;
}

void UFLibAssetManageHelperEx::GetAllInValidAssetInProject(FAssetDependenciesInfo InAllDependencies, TArray<FString> &OutInValidAsset)
{
	TArray<FString> Keys;
	InAllDependencies.mDependencies.GetKeys(Keys);
	for (const auto& ModuleItem : Keys)
	{
		// ignore search /Script Asset
		if (ModuleItem == TEXT("Script"))
			continue;
		FAssetDependenciesDetail ModuleDependencies = InAllDependencies.mDependencies[ModuleItem];

		TArray<FString> ModuleAssetList;
		ModuleDependencies.mDependAssetDetails.GetKeys(ModuleAssetList);
		for (const auto& AssetLongPackageName : ModuleAssetList)
		{
			FString AssetPackagePath;
			if(!UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(AssetLongPackageName,AssetPackagePath))
				continue;
			FString AssetAbsPath = UFLibAssetManageHelperEx::ConvVirtualToAbsPath(AssetPackagePath);
			if (!FPaths::FileExists(AssetAbsPath))
			{
				OutInValidAsset.Add(AssetLongPackageName);
			}
		}
	}
}
const FAssetPackageData* UFLibAssetManageHelperEx::GetPackageDataByPackagePath(const FString& InPackagePath)
{

	if (InPackagePath.IsEmpty())
		return NULL;
	if (!InPackagePath.IsEmpty())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FString TargetLongPackageName = UFLibAssetManageHelperEx::GetLongPackageNameFromPackagePath(InPackagePath);

		if(FPackageName::DoesPackageExist(TargetLongPackageName))
		{
			FAssetPackageData* AssetPackageData = const_cast<FAssetPackageData*>(AssetRegistryModule.Get().GetAssetPackageData(*TargetLongPackageName));
			if (AssetPackageData != nullptr)
			{
				return AssetPackageData;
			}
		}
	}

	return NULL;
}

bool UFLibAssetManageHelperEx::ConvLongPackageNameToCookedPath(const FString& InProjectAbsDir, const FString& InPlatformName, const FString& InLongPackageName, TArray<FString>& OutCookedAssetPath, TArray<FString>& OutCookedAssetRelativePath)
{
	if (!FPaths::DirectoryExists(InProjectAbsDir) || !IsValidPlatform(InPlatformName))
		return false;

	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	FString CookedRootDir = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"), InPlatformName);
	FString ProjectName = FApp::GetProjectName();
	FString AssetPackagePath;
	UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(InLongPackageName,AssetPackagePath);
	FString AssetAbsPath = UFLibAssetManageHelperEx::ConvVirtualToAbsPath(AssetPackagePath);

	FString AssetModuleName = InLongPackageName;
	{
		AssetModuleName.RemoveFromStart(TEXT("/"));
		int32 secondSlashIndex = -1;
		AssetModuleName.FindChar('/', secondSlashIndex);
		AssetModuleName = UKismetStringLibrary::GetSubstring(AssetModuleName, 0, secondSlashIndex);
	}

	bool bIsEngineModule = false;
	FString AssetBelongModuleBaseDir;
	FString AssetCookedNotPostfixPath;
	{

		if (UFLibAssetManageHelperEx::GetEnableModuleAbsDir(AssetModuleName, AssetBelongModuleBaseDir))
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

	FFillArrayDirectoryVisitor FileVisitor;
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


bool UFLibAssetManageHelperEx::GetCookCommandFromAssetDependencies(const FString& InProjectDir, const FString& InPlatformName, const FAssetDependenciesInfo& InAssetDependencies, const TArray<FString> &InCookParams, TArray<FString>& OutCookCommand)
{
	if (!FPaths::DirectoryExists(InProjectDir) || !UFLibAssetManageHelperEx::IsValidPlatform(InPlatformName))
		return false;
	OutCookCommand.Empty();
	// TArray<FString> resault;
	TArray<FString> Keys;
	InAssetDependencies.mDependencies.GetKeys(Keys);

	for (const auto& Key : Keys)
	{
		if (Key.Equals(TEXT("Script")))
			continue;
		TArray<FString> AssetList;
		InAssetDependencies.mDependencies.Find(Key)->mDependAssetDetails.GetKeys(AssetList);
		for (const auto& AssetLongPackageName : AssetList)
		{
			TArray<FString> CookedAssetAbsPath;
			TArray<FString> CookedAssetRelativePath;
			TArray<FString> FinalCookedCommand;
			if (UFLibAssetManageHelperEx::ConvLongPackageNameToCookedPath(InProjectDir, InPlatformName, AssetLongPackageName, CookedAssetAbsPath, CookedAssetRelativePath))
			{
				if (UFLibAssetManageHelperEx::CombineCookedAssetCommand(CookedAssetAbsPath, CookedAssetRelativePath, InCookParams, FinalCookedCommand))
				{
					OutCookCommand.Append(FinalCookedCommand);
				}
			}

		}
	}
	return true;
}

bool UFLibAssetManageHelperEx::CombineCookedAssetCommand(const TArray<FString> &InAbsPath, const TArray<FString>& InRelativePath, const TArray<FString>& InParams, TArray<FString>& OutCommand)
{
	OutCommand.Empty();
	if (InAbsPath.Num() != InRelativePath.Num())
		return false;
	int32 AssetNum = InAbsPath.Num();
	for (int32 index = 0; index < AssetNum; ++index)
	{
		FString CurrentCommand = TEXT("\"") + InAbsPath[index] + TEXT("\" \"") + InRelativePath[index] + TEXT("\"");
		for (const auto& Param : InParams)
		{
			CurrentCommand += TEXT(" ") + Param;
		}
		OutCommand.Add(CurrentCommand);
	}
	return true;
}

bool UFLibAssetManageHelperEx::ExportCookPakCommandToFile(const TArray<FString>& InCommand, const FString& InFile)
{
	return FFileHelper::SaveStringArrayToFile(InCommand, *InFile);
}

/*
	TOOLs Set Implementation
*/

bool UFLibAssetManageHelperEx::SerializeAssetDependenciesToJson(const FAssetDependenciesInfo& InAssetDependencies, FString& OutJsonStr)
{
	OutJsonStr.Empty();
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> RootJsonObject;
	if (!UFLibAssetManageHelperEx::SerializeAssetDependenciesToJsonObject(InAssetDependencies, RootJsonObject))
		return bRunStatus;
	if (RootJsonObject.IsValid())
	{
		auto JsonWriter = TJsonWriterFactory<>::Create(&OutJsonStr);
		FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter);
		bRunStatus = true;
	}
	return bRunStatus;
}


bool UFLibAssetManageHelperEx::SerializeAssetDependenciesToJsonObject(const FAssetDependenciesInfo& InAssetDependencies, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);
	{
		// collect all module name
		TArray<FString> AssetCategoryList;
		InAssetDependencies.mDependencies.GetKeys(AssetCategoryList);
		{
			TArray<TSharedPtr<FJsonValue>> JsonCategoryList;
			for (const auto& AssetCategoryItem : AssetCategoryList)
			{
				JsonCategoryList.Add(MakeShareable(new FJsonValueString(AssetCategoryItem)));
			}
			RootJsonObject->SetArrayField(JSON_MODULE_LIST_SECTION_NAME, JsonCategoryList);
		}


		// serialize asset list
		TSharedPtr<FJsonObject> AssetListJsonObject = MakeShareable(new FJsonObject);
		for (const auto& AssetCategoryItem : AssetCategoryList)
		{

			// TSharedPtr<FJsonObject> CategoryJsonObject = MakeShareable(new FJsonObject);

			TArray<TSharedPtr<FJsonValue>> CategoryAssetListJsonEntity;
			const FAssetDependenciesDetail& CategortItem = InAssetDependencies.mDependencies[AssetCategoryItem];
			TArray<TSharedPtr<FJsonValue>> AssetDetialsJsonObject;
			TArray<FString> CategoryAssetList;
			CategortItem.mDependAssetDetails.GetKeys(CategoryAssetList);

			for (const auto& AssetItem : CategoryAssetList)
			{
				CategoryAssetListJsonEntity.Add(MakeShareable(new FJsonValueString(AssetItem)));

			}
			// CategoryJsonObject->SetArrayField(AssetCategoryItem, CategoryAssetListJsonEntity);
			AssetListJsonObject->SetArrayField(AssetCategoryItem, CategoryAssetListJsonEntity);



		}

		// collect all invalid asset
		{
			TArray<FString> AllInValidAssetList;
			UFLibAssetManageHelperEx::GetAllInValidAssetInProject(InAssetDependencies, AllInValidAssetList);
			if (AllInValidAssetList.Num() > 0)
			{
				TArray<TSharedPtr<FJsonValue>> JsonInvalidAssetList;
				for (const auto& InValidAssetItem : AllInValidAssetList)
				{
					JsonInvalidAssetList.Add(MakeShareable(new FJsonValueString(InValidAssetItem)));
				}
				AssetListJsonObject->SetArrayField(JSON_ALL_INVALID_ASSET_SECTION_NAME, JsonInvalidAssetList);
			}
		}

		RootJsonObject->SetObjectField(JSON_ALL_ASSETS_LIST_SECTION_NAME, AssetListJsonObject);

		// serilize asset detail
		TSharedPtr<FJsonObject> AssetDetailsListJsonObject = MakeShareable(new FJsonObject);
		for (const auto& AssetCategoryItem : AssetCategoryList)
		{
			TSharedPtr<FJsonObject> CurrentCategoryJsonObject = MakeShareable(new FJsonObject);
			const FAssetDependenciesDetail& CategortItem = InAssetDependencies.mDependencies[AssetCategoryItem];
			TArray<FString> AssetList;
			CategortItem.mDependAssetDetails.GetKeys(AssetList);
			for (const auto& AssetLongPackageName : AssetList)
			{
				const FAssetDetail& CurrentAssetDetail = *CategortItem.mDependAssetDetails.Find(AssetLongPackageName);

				FString CurrentPackageName = FSoftObjectPath(CurrentAssetDetail.mPackagePath).GetLongPackageName();
				TSharedPtr<FJsonObject> CurrentJsonObject = UFLibAssetManageHelperEx::SerilizeAssetDetial(CurrentAssetDetail);
				CurrentCategoryJsonObject->SetObjectField(CurrentPackageName, CurrentJsonObject);
			}
			AssetDetailsListJsonObject->SetObjectField(AssetCategoryItem, CurrentCategoryJsonObject);
		}

		RootJsonObject->SetObjectField(JSON_ALL_ASSETS_Detail_SECTION_NAME, AssetDetailsListJsonObject);
	}
	OutJsonObject = RootJsonObject;
	return true;
}

bool UFLibAssetManageHelperEx::DeserializeAssetDependencies(const FString& InStream, FAssetDependenciesInfo& OutAssetDependencies)
{
	bool bRunStatus = false;
	if (InStream.IsEmpty()) return false;

	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InStream);
	TSharedPtr<FJsonObject> RootJsonObject;;
	if (FJsonSerializer::Deserialize(JsonReader, RootJsonObject))
	{
		bRunStatus = UFLibAssetManageHelperEx::DeserializeAssetDependenciesForJsonObject(RootJsonObject, OutAssetDependencies);
	}
	return true;
}
  
bool UFLibAssetManageHelperEx::DeserializeAssetDependenciesForJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FAssetDependenciesInfo& OutAssetDependencies)
{
	bool bRunStatus = false;
	if (InJsonObject.IsValid())
	{
		TArray<TSharedPtr<FJsonValue>> JsonModuleList = InJsonObject->GetArrayField(JSON_MODULE_LIST_SECTION_NAME);
		for (const auto& JsonModuleItem : JsonModuleList)
		{

			FString ModuleName = JsonModuleItem->AsString();

			//deserialize assets list
			TArray<FString> AssetList;
			{
				TSharedPtr<FJsonObject> AssetsListJsonObject = InJsonObject->GetObjectField(JSON_ALL_ASSETS_LIST_SECTION_NAME);
				TArray<TSharedPtr<FJsonValue>> JsonAssetList = AssetsListJsonObject->GetArrayField(ModuleName);

				for (const auto& JsonAssetItem : JsonAssetList)
				{
					FString AssetInfo = JsonAssetItem->AsString();
					AssetList.Add(AssetInfo);
				}
			}
			// deserialize Assets detail
			TMap<FString,FAssetDetail> ModuleAssetDetail;
			{
				TSharedPtr<FJsonObject> AssetsDetailJsonObject = InJsonObject->GetObjectField(JSON_ALL_ASSETS_Detail_SECTION_NAME);
				TSharedPtr<FJsonObject> ModuleAssetDetailJsonObject = AssetsDetailJsonObject->GetObjectField(ModuleName);

				for (const auto& AssetLongPackageName : AssetList)
				{
					TSharedPtr<FJsonObject> CurrentAssetDetailJsonObject = ModuleAssetDetailJsonObject->GetObjectField(AssetLongPackageName);

					FAssetDetail CurrentAssetDetail;
					if (UFLibAssetManageHelperEx::DeserilizeAssetDetial(CurrentAssetDetailJsonObject, CurrentAssetDetail))
					{
						ModuleAssetDetail.Add(AssetLongPackageName,CurrentAssetDetail);
					}	
				}

			}
			OutAssetDependencies.mDependencies.Add(ModuleName, FAssetDependenciesDetail(ModuleName, ModuleAssetDetail));
		}

		bRunStatus = true;
	}

	return bRunStatus;
}

TSharedPtr<FJsonObject> UFLibAssetManageHelperEx::SerilizeAssetDetial(const FAssetDetail& InAssetDetail)
{
	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);
	{
		RootJsonObject->SetStringField("PackagePath", InAssetDetail.mPackagePath);
		RootJsonObject->SetStringField("AssetType", InAssetDetail.mAssetType);
		RootJsonObject->SetStringField("AssetGUID", InAssetDetail.mGuid);
	}

	return RootJsonObject;
}

bool UFLibAssetManageHelperEx::DeserilizeAssetDetial(TSharedPtr<FJsonObject>& InJsonObject, FAssetDetail& OutAssetDetail)
{
	bool bRunStatus = false;
	if (InJsonObject.IsValid())
	{
		OutAssetDetail.mPackagePath = InJsonObject->GetStringField(TEXT("PackagePath"));
		OutAssetDetail.mAssetType = InJsonObject->GetStringField(TEXT("AssetType"));
		OutAssetDetail.mGuid = InJsonObject->GetStringField(TEXT("AssetGUID"));
		bRunStatus = true;
	}
	return bRunStatus;
}

FString UFLibAssetManageHelperEx::SerializeAssetDetialArrayToString(const TArray<FAssetDetail>& InAssetDetialList)
{
	TSharedPtr<FJsonObject> RootJsonObject=MakeShareable(new FJsonObject);

	for (const auto& AssetDetial : InAssetDetialList)
	{
		RootJsonObject->SetObjectField(AssetDetial.mPackagePath, UFLibAssetManageHelperEx::SerilizeAssetDetial(AssetDetial));
	}

	FString OutJsonStr;
	auto JsonWriter = TJsonWriterFactory<>::Create(&OutJsonStr);
	FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter);
	return OutJsonStr;
}

bool UFLibAssetManageHelperEx::SaveStringToFile(const FString& InFile, const FString& InString)
{
	return FFileHelper::SaveStringToFile(InString, *InFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool UFLibAssetManageHelperEx::LoadFileToString(const FString& InFile, FString& OutString)
{
	return FFileHelper::LoadFileToString(OutString, *InFile);
}

bool UFLibAssetManageHelperEx::GetPluginModuleAbsDir(const FString& InPluginModuleName, FString& OutPath)
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

void UFLibAssetManageHelperEx::GetAllEnabledModuleName(TArray<FString>& OutEnabledModule)
{
	OutEnabledModule.Reset();
	OutEnabledModule.Add(TEXT("Game"));
	OutEnabledModule.Add(TEXT("Engine"));
	TArray<TSharedRef<IPlugin>> AllPlugin = IPluginManager::Get().GetEnabledPlugins();

	for (const auto& PluginItem : AllPlugin)
	{
		OutEnabledModule.Add(PluginItem.Get().GetName());
	}
}

bool UFLibAssetManageHelperEx::GetEnableModuleAbsDir(const FString& InModuleName, FString& OutPath)
{
	if (InModuleName.Equals(TEXT("Engine")))
	{
		OutPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
		return true;
	}
	if (InModuleName.Equals(TEXT("Game")))
	{
		OutPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		return true;
	}
	return UFLibAssetManageHelperEx::GetPluginModuleAbsDir(InModuleName, OutPath);

}

FString UFLibAssetManageHelperEx::GetAssetBelongModuleName(const FString& InAssetRelativePath)
{
	FString LongDependentPackageName = InAssetRelativePath;

	int32 BelongModuleNameEndIndex = LongDependentPackageName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1);
	FString BelongModuleName = UKismetStringLibrary::GetSubstring(LongDependentPackageName, 1, BelongModuleNameEndIndex - 1);// (LongDependentPackageName, BelongModuleNameEndIndex);

	return BelongModuleName;
}

bool UFLibAssetManageHelperEx::IsValidPlatform(const FString& PlatformName)
{
	for (const auto& PlatformItem : UFLibAssetManageHelperEx::GetAllTargetPlatform())
	{
		if (PlatformItem.Equals(PlatformName))
		{
			return true;
		}
	}
	return false;
}

TArray<FString> UFLibAssetManageHelperEx::GetAllTargetPlatform()
{
#ifdef __DEVELOPER_MODE__
	TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();
	TArray<FString> result;

	for (const auto& PlatformItem : Platforms)
	{
		result.Add(PlatformItem->PlatformName());
	}

#else
	TArray<FString> result = {
		"AllDesktop",
		"MacClient",
		"MacNoEditor",
		"MacServer",
		"Mac",
		"WindowsClient",
		"WindowsNoEditor",
		"WindowsServer",
		"Windows",
		"Android",
		"Android_ASTC",
		"Android_ATC",
		"Android_DXT",
		"Android_ETC1",
		"Android_ETC1a",
		"Android_ETC2",
		"Android_PVRTC",
		"AndroidClient",
		"Android_ASTCClient",
		"Android_ATCClient",
		"Android_DXTClient",
		"Android_ETC1Client",
		"Android_ETC1aClient",
		"Android_ETC2Client",
		"Android_PVRTCClient",
		"Android_Multi",
		"Android_MultiClient",
		"HTML5",
		"IOSClient",
		"IOS",
		"TVOSClient",
		"TVOS",
		"LinuxClient",
		"LinuxNoEditor",
		"LinuxServer",
		"Linux",
		"Lumin",
		"LuminClient"
	};

#endif
	return result;
}


bool UFLibAssetManageHelperEx::FindFilesRecursive(const FString& InStartDir, TArray<FString>& OutFileList, bool InRecursive)
{
	TArray<FString> CurrentFolderFileList;
	if (!FPaths::DirectoryExists(InStartDir))
		return false;

	FFillArrayDirectoryVisitor FileVisitor;
	IFileManager::Get().IterateDirectoryRecursively(*InStartDir, FileVisitor);

	OutFileList.Append(FileVisitor.Files);

	return true;
}

FString UFLibAssetManageHelperEx::ConvPath_Slash2BackSlash(const FString& InPath)
{
	FString ResaultPath;
	TArray<FString> OutArray;

	InPath.ParseIntoArray(OutArray, TEXT("/"));
	int32 OutArrayNum = OutArray.Num();
	for (int32 Index = 0; Index < OutArrayNum; ++Index)
	{
		if (!OutArray[Index].IsEmpty() && Index < OutArrayNum - 1)/* && FPaths::DirectoryExists(ResaultPath + item)*/
		{
			ResaultPath.Append(OutArray[Index]);
			ResaultPath.Append(TEXT("\\"));
		}
		else {
			ResaultPath.Append(OutArray[Index]);
		}
	}
	return ResaultPath;
}

FString UFLibAssetManageHelperEx::ConvPath_BackSlash2Slash(const FString& InPath)
{
	FString ResaultPath;
	TArray<FString> OutArray;
	InPath.ParseIntoArray(OutArray, TEXT("\\"));
	if (OutArray.Num() == 1 && OutArray[0] == InPath)
	{
		InPath.ParseIntoArray(OutArray, TEXT("/"));
	}
	int32 OutArrayNum = OutArray.Num();
	for (int32 Index = 0; Index < OutArrayNum; ++Index)
	{
		if (!OutArray[Index].IsEmpty() && Index < OutArrayNum - 1)/* && FPaths::DirectoryExists(ResaultPath + item)*/
		{
			ResaultPath.Append(OutArray[Index]);
			ResaultPath.Append(TEXT("/"));
		}
		else {
			ResaultPath.Append(OutArray[Index]);
		}
	}
	return ResaultPath;
}