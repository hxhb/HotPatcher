// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibAssetManageHelper.h"
#include "SoftObjectPath.h"
#include "Kismet/KismetStringLibrary.h"

void UFlibAssetManageHelper::GetAssetDependencies(const FString& InAsset, FAssetDependenciesInfo& OutDependInfo)
{
	if (InAsset.IsEmpty())
		return;

	FStringAssetReference AssetRef = FStringAssetReference(InAsset);
	if (!AssetRef.IsValid())
		return;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FStringAssetReference InAssetRef = InAsset;
	FString TargetLongPackageName = InAssetRef.GetLongPackageName();
	// UE_LOG(LogTemp, Log, TEXT("TargetLongPackageName is %s."), *TargetLongPackageName);
	if (FPackageName::DoesPackageExist(TargetLongPackageName))
	{
		{
			TArray<FAssetData> AssetDataList;
			bool bResault = AssetRegistryModule.Get().GetAssetsByPackageName(FName(*TargetLongPackageName), AssetDataList);
			if (!bResault || !AssetDataList.Num())
			{
				UE_LOG(LogTemp, Error, TEXT("Faild to Parser AssetData of %s, please check."), *TargetLongPackageName);
				return;
			}
			if (AssetDataList.Num() > 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Got mulitple AssetData of %s,please check."), *TargetLongPackageName);
			}
		}
		UFlibAssetManageHelper::GatherAssetDependicesInfoRecursively(AssetRegistryModule, TargetLongPackageName, OutDependInfo);
	}

}

void UFlibAssetManageHelper::GetAssetDependenciesBySoftRef(TSoftObjectPtr<UObject> InAssetRef, FAssetDependenciesInfo& OutDependicesInfo)
{
	if (!InAssetRef.IsValid())
		return;
	UFlibAssetManageHelper::GetAssetDependencies(InAssetRef.ToString(), OutDependicesInfo);

}

TArray<FString> UFlibAssetManageHelper::GetAssetAllDependencies(const FString& InAsset)
{
	TArray<FString> Resault;
	FAssetDependenciesInfo local_DependenciesInfo;
	GetAssetDependencies(InAsset, local_DependenciesInfo);

	TArray<FString> Keys;
	local_DependenciesInfo.mDependencies.GetKeys(Keys);
	for (auto& Key : Keys)
	{
		Resault.Append(local_DependenciesInfo.mDependencies[Key].mDependAsset);
	}
	return Resault;
}

void UFlibAssetManageHelper::GatherAssetDependicesInfoRecursively(
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

			int32 BelongModuleNameEndIndex = LongDependentPackageName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1);
			FString BelongModuleName = UKismetStringLibrary::GetSubstring(LongDependentPackageName, 1, BelongModuleNameEndIndex-1);// (LongDependentPackageName, BelongModuleNameEndIndex);
			
			// add a new asset to module category
			auto AddNewAssetItemLambda = [&InAssetRegistryModule,&OutDependencies](
				FAssetDependenciesDetail& ModuleAssetDependDetail, 
				FString AssetPackageName)
			{
				if (ModuleAssetDependDetail.mDependAsset.Find(AssetPackageName) == INDEX_NONE)
				{
					ModuleAssetDependDetail.mDependAsset.Add(AssetPackageName);
					GatherAssetDependicesInfoRecursively(InAssetRegistryModule, AssetPackageName, OutDependencies);
				}
			};

			if (OutDependencies.mDependencies.Contains(BelongModuleName))
			{
				// UE_LOG(LogTemp, Log, TEXT("Belong Module is %s,Asset is %s"), *BelongModuleName, *LongDependentPackageName);
				FAssetDependenciesDetail* ModuleCategory=OutDependencies.mDependencies.Find(BelongModuleName);
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

FString UFlibAssetManageHelper::ConvGameAssetRelativePathToAbs(const FString& InProjectDir, const FString& InRelativePath)
{
	FString AssetRelativePath = UFlibAssetManageHelper::ConvPath_Slash2BackSlash(InRelativePath);
	FString Resault;
	if (!FPaths::DirectoryExists(InProjectDir))
		return Resault;

	if (AssetRelativePath.StartsWith(TEXT("/Game")))
	{
		FString ContentRelative = UKismetStringLibrary::GetSubstring(AssetRelativePath, 5, AssetRelativePath.Len());
		Resault = FPaths::Combine(InProjectDir, TEXT("Content"), ContentRelative);
	}
	bool bIsDir = FPaths::DirectoryExists(Resault);

	if (bIsDir)
	{
		return Resault;
	}

	FString AssetName;
	{
		int32 SearchDotIndex = -1;
		if (Resault.FindLastChar('.', SearchDotIndex))
		{
			AssetName = UKismetStringLibrary::GetSubstring(Resault, SearchDotIndex + 1, Resault.Len() - SearchDotIndex);
			Resault.RemoveAt(SearchDotIndex, Resault.Len() - SearchDotIndex);
		}
	}

	TArray<FString> localFindFiles;
	FString SearchDir;
	int32 FindLastBackSlant = -1;
	if (Resault.FindLastChar('/', FindLastBackSlant))
	{
		SearchDir = UKismetStringLibrary::GetSubstring(Resault, 0, FindLastBackSlant);
	}

	IFileManager::Get().FindFiles(localFindFiles, *SearchDir, nullptr);

	for (const auto& Item : localFindFiles)
	{
		if (Item.Contains(AssetName) && Item[AssetName.Len()] == '.')
		{
			Resault = SearchDir / Item;
			break;
		}
	}

	bool bIsFile = FPaths::FileExists(Resault);
	return bIsFile ? Resault : TEXT("");
	
}

FString UFlibAssetManageHelper::ConvGameAssetAbsPathToRelative(const FString& InProjectDir, const FString& InAbsPath)
{
	FString AssetAbsPath = UFlibAssetManageHelper::ConvPath_Slash2BackSlash(InAbsPath);

	FString Resault;
	if (!(FPaths::DirectoryExists(InProjectDir) || FPaths::DirectoryExists(AssetAbsPath)) ||
		!AssetAbsPath.Contains(InProjectDir)) return Resault;

	bool bIsDir = FPaths::DirectoryExists(AssetAbsPath);
	bool bIsFile = FPaths::FileExists(AssetAbsPath);

	FString RelativePath = UKismetStringLibrary::GetSubstring(AssetAbsPath, InProjectDir.Len() + FString(TEXT("Content")).Len(), AssetAbsPath.Len() - InProjectDir.Len());
	if (bIsDir)
	{
		Resault = TEXT("/Game") + RelativePath;
	}

	if (bIsFile)
	{
		Resault = TEXT("/Game") + RelativePath;
		int32 FindIndex;
		if (Resault.FindLastChar('.', FindIndex))
		{
			Resault.RemoveAt(FindIndex, Resault.Len() - FindIndex);

			int32 LastBackSlant;
			Resault.FindLastChar('/', LastBackSlant);
			Resault += TEXT(".") + UKismetStringLibrary::GetSubstring(Resault, LastBackSlant + 1, Resault.Len() - LastBackSlant);
		}
	}

	return Resault;
}

FString UFlibAssetManageHelper::ConvPath_Slash2BackSlash(const FString& InPath)
{
	FString ResaultPath;
	TArray<FString> OutArray;
	InPath.ParseIntoArray(OutArray, TEXT("\\"));
	if (OutArray.Num() == 1 && OutArray[0] == InPath)
	{
		InPath.ParseIntoArray(OutArray, TEXT("/"));
	}
	for (const auto& item : OutArray)
	{
		if (FPaths::DirectoryExists(ResaultPath + item))
		{
			ResaultPath.Append(item);
			ResaultPath.Append(TEXT("\\"));
		}
		else {
			ResaultPath.Append(item);
		}
	}
	// Recursive
	{
		TArray<FString> RecursiveResault;
		ResaultPath.ParseIntoArray(RecursiveResault, TEXT("\\"));
		if (RecursiveResault.Num() > 0)
		{
			return ConvPath_Slash2BackSlash(ResaultPath);
		}
		else {
			return ResaultPath;
		}
	}
	
}
