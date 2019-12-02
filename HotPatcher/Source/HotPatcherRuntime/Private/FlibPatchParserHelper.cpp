// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibPatchParserHelper.h"
#include "FlibAssetManageHelperEx.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/EngineTypes.h"
TArray<FString> UFlibPatchParserHelper::GetAvailableMaps(FString GameName, bool IncludeEngineMaps, bool Sorted)
{
	TArray<FString> Result;
	TArray<FString> EnginemapNames;
	TArray<FString> ProjectMapNames;

	const FString WildCard = FString::Printf(TEXT("*%s"), *FPackageName::GetMapPackageExtension());

	// Scan all Content folder, because not all projects follow Content/Maps convention
	IFileManager::Get().FindFilesRecursive(ProjectMapNames, *FPaths::Combine(*FPaths::RootDir(), *GameName, TEXT("Content")), *WildCard, true, false);

	// didn't find any, let's check the base GameName just in case it is a full path
	if (ProjectMapNames.Num() == 0)
	{
		IFileManager::Get().FindFilesRecursive(ProjectMapNames, *FPaths::Combine(*GameName, TEXT("Content")), *WildCard, true, false);
	}

	for (int32 i = 0; i < ProjectMapNames.Num(); i++)
	{
		Result.Add(FPaths::GetBaseFilename(ProjectMapNames[i]));
	}

	if (IncludeEngineMaps)
	{
		IFileManager::Get().FindFilesRecursive(EnginemapNames, *FPaths::Combine(*FPaths::RootDir(), TEXT("Engine"), TEXT("Content"), TEXT("Maps")), *WildCard, true, false);

		for (int32 i = 0; i < EnginemapNames.Num(); i++)
		{
			Result.Add(FPaths::GetBaseFilename(EnginemapNames[i]));
		}
	}

	if (Sorted)
	{
		Result.Sort();
	}

	return Result;
}

FHotPatcherVersion UFlibPatchParserHelper::ExportReleaseVersionInfo(const FString& InVersionId, const FString& InBaseVersion,const FString& InDate, const TArray<FString>& InIncludeFilter, const TArray<FString>& InIgnoreFilter)
{
	FHotPatcherVersion ExportVersion;
	{
		ExportVersion.VersionId = InVersionId;
		ExportVersion.Date = InDate;
		ExportVersion.BaseVersionId = InBaseVersion;
	}
	

	// add Include Filter
	{
		TArray<FString> AllIncludeFilter;
		for (const auto& Filter : InIncludeFilter)
		{
			AllIncludeFilter.AddUnique(Filter);
			ExportVersion.IncludeFilter.AddUnique(Filter);
		}
	}
	// add Ignore Filter
	{
		TArray<FString> AllIgnoreFilter;
		for (const auto& Filter : InIgnoreFilter)
		{
			AllIgnoreFilter.AddUnique(Filter);
			ExportVersion.IgnoreFilter.AddUnique(Filter);
		}
	}

	TArray<FAssetDetail> FinalAssetsList;
	{
		TArray<FAssetDetail> AllHasRefAssets;
		{
			TArray<FAssetDetail> AllAssets;
			UFLibAssetManageHelperEx::GetAssetsList(ExportVersion.IncludeFilter, AllAssets);
			TArray<FAssetDetail> AllDontHasRefAssets;
			UFLibAssetManageHelperEx::FilterNoRefAssets(AllAssets, AllHasRefAssets, AllDontHasRefAssets);
		}
		// 剔除ignore filter中指定的资源
		if (ExportVersion.IgnoreFilter.Num() > 0)
		{
			for (const auto& AssetDetail : AllHasRefAssets)
			{
				bool bIsIgnore = false;
				for (const auto& IgnoreFilter : ExportVersion.IgnoreFilter)
				{
					if (AssetDetail.mPackagePath.StartsWith(IgnoreFilter))
					{
						bIsIgnore = true;
						break;
					}
				}
				if (!bIsIgnore)
				{
					FinalAssetsList.Add(AssetDetail);
				}
			}
		}
		else
		{
			FinalAssetsList = AllHasRefAssets;
		}
	}

	// 分析资源依赖
	FAssetDependenciesInfo FinalResault;
	if (FinalAssetsList.Num())
	{
		FAssetDependenciesInfo AllAssetListInfo;
		UFLibAssetManageHelperEx::CombineAssetsDetailAsFAssetDepenInfo(FinalAssetsList, AllAssetListInfo);

		FAssetDependenciesInfo AssetDependencies;
		UFLibAssetManageHelperEx::GetAssetListDependenciesForAssetDetail(FinalAssetsList, AssetDependencies);

		FinalResault = UFLibAssetManageHelperEx::CombineAssetDependencies(AllAssetListInfo, AssetDependencies);

	}
	ExportVersion.AssetInfo = FinalResault;

	return ExportVersion;
}

bool UFlibPatchParserHelper::SerializeHotPatcherVersionToString(const FHotPatcherVersion& InVersion, FString& OutResault)
{
	bool bRunStatus = false;

	{
		TSharedPtr<FJsonObject> JsonObject;
		if (UFlibPatchParserHelper::SerializeHotPatcherVersionToJsonObject(InVersion, JsonObject) && JsonObject.IsValid())
		{
			auto JsonWriter = TJsonWriterFactory<>::Create(&OutResault);
			FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
			bRunStatus = true;
		}
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializeHotPatcherVersionToJsonObject(const FHotPatcherVersion& InVersion, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);
	{
		RootJsonObject->SetStringField(TEXT("VersionId"), InVersion.VersionId);
		RootJsonObject->SetStringField(TEXT("BaseVersionId"), InVersion.BaseVersionId);
		RootJsonObject->SetStringField(TEXT("Date"), InVersion.Date);

		// serialize asset include filter
		{
			TArray<TSharedPtr<FJsonValue>> AllIncludeFilterJsonObj;
			for (const auto& Filter : InVersion.IncludeFilter)
			{
				AllIncludeFilterJsonObj.Add(MakeShareable(new FJsonValueString(Filter)));
			}
			RootJsonObject->SetArrayField(TEXT("IncludeFilter"), AllIncludeFilterJsonObj);

		}
		// serialize asset Ignore filter
		{
			TArray<TSharedPtr<FJsonValue>> AllIgnoreFilterJsonObj;
			for (const auto& Filter : InVersion.IgnoreFilter)
			{
				AllIgnoreFilterJsonObj.Add(MakeShareable(new FJsonValueString(Filter)));
			}
			RootJsonObject->SetArrayField(TEXT("IgnoreFilter"), AllIgnoreFilterJsonObj);

		}

		TSharedPtr<FJsonObject> AssetInfoJsonObject;
		bool bSerializeAssetInfoStatus = UFLibAssetManageHelperEx::SerializeAssetDependenciesToJsonObject(InVersion.AssetInfo, AssetInfoJsonObject);

		RootJsonObject->SetObjectField(TEXT("AssetInfo"), AssetInfoJsonObject);
		bRunStatus = true;
	}
	OutJsonObject = RootJsonObject;
	return bRunStatus;
}

bool UFlibPatchParserHelper::DeserializeHotPatcherVersionFromString(const FString& InStringContent, FHotPatcherVersion& OutVersion)
{
	bool bRunStatus = false;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InStringContent);
	TSharedPtr<FJsonObject> DeserializeJsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, DeserializeJsonObject))
	{
		bRunStatus = UFlibPatchParserHelper::DeSerializeHotPatcherVersionFromJsonObject(DeserializeJsonObject, OutVersion);
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::DeSerializeHotPatcherVersionFromJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FHotPatcherVersion& OutVersion)
{
	bool bRunStatus = false;
	if (InJsonObject.IsValid())
	{

		OutVersion.VersionId = InJsonObject->GetStringField(TEXT("VersionId"));
		OutVersion.BaseVersionId = InJsonObject->GetStringField(TEXT("BaseVersionId"));
		OutVersion.Date = InJsonObject->GetStringField(TEXT("Date"));


		// deserialize asset include filter
		{
			TArray<TSharedPtr<FJsonValue>> AllIncludeFilterJsonObj;
			AllIncludeFilterJsonObj = InJsonObject->GetArrayField(TEXT("IncludeFilter"));
			for (const auto& Filter : AllIncludeFilterJsonObj)
			{
				OutVersion.IncludeFilter.AddUnique(Filter->AsString());
			}
		}
		// deserialize asset Ignore filter
		{
			TArray<TSharedPtr<FJsonValue>> AllIgnoreFilterJsonObj;
			AllIgnoreFilterJsonObj = InJsonObject->GetArrayField(TEXT("IncludeFilter"));
			for (const auto& Filter : AllIgnoreFilterJsonObj)
			{
				OutVersion.IgnoreFilter.AddUnique(Filter->AsString());
			}
		}

		FAssetDependenciesInfo DeserialAssetInfo;
		if (UFLibAssetManageHelperEx::DeserializeAssetDependenciesForJsonObject(InJsonObject->GetObjectField(TEXT("AssetInfo")),DeserialAssetInfo))
		{
			OutVersion.AssetInfo = DeserialAssetInfo;
		}
		bRunStatus = true;
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::DiffVersion(
	const FAssetDependenciesInfo& InNewVersion,
	const FAssetDependenciesInfo& InBaseVersion,
	FAssetDependenciesInfo& OutAddAsset,
	FAssetDependenciesInfo& OutModifyAsset,
	FAssetDependenciesInfo& OutDeleteAsset
)
{
	FAssetDependenciesInfo result;
	TArray<FString> AddAsset;
	TArray<FString> ModifyAsset;
	TArray<FString> DeleteAsset;

	{
		TArray<FString> InNewAssetModuleKeyList;
		InNewVersion.mDependencies.GetKeys(InNewAssetModuleKeyList);

		TArray<FString> InBaseAssetModuleKeysList;
		InBaseVersion.mDependencies.GetKeys(InBaseAssetModuleKeysList);

		// Parser Add new asset
		for (const auto& NewVersionAssetModule : InNewAssetModuleKeyList)
		{
			// is a new mdoule?
			if (!InBaseAssetModuleKeysList.Contains(NewVersionAssetModule))
			{
				OutAddAsset.mDependencies.Add(NewVersionAssetModule, *InNewVersion.mDependencies.Find(NewVersionAssetModule));
				continue;
			}
			
			{
				

				TArray<FString> NewVersionDependAssetsList;
				InNewVersion.mDependencies.Find(NewVersionAssetModule)->mDependAssetDetails.GetKeys(NewVersionDependAssetsList);

				TArray<FString> BaseVersionDependAssetsList;
				InBaseVersion.mDependencies.Find(NewVersionAssetModule)->mDependAssetDetails.GetKeys(BaseVersionDependAssetsList);

				const TMap<FString,FAssetDetail>& NewVersionAssetModuleDetail = InNewVersion.mDependencies.Find(NewVersionAssetModule)->mDependAssetDetails;
				TArray<FString> CurrentModuleAssetList;
				NewVersionAssetModuleDetail.GetKeys(CurrentModuleAssetList);

				// add to TArray<FString>
				for (const auto& NewAssetItem : CurrentModuleAssetList)
				{
					if (!BaseVersionDependAssetsList.Contains(NewAssetItem))
					{
						FString BelongModuneName = UFLibAssetManageHelperEx::GetAssetBelongModuleName(NewAssetItem);
						if (!OutAddAsset.mDependencies.Contains(BelongModuneName))
						{
							OutAddAsset.mDependencies.Add(BelongModuneName, FAssetDependenciesDetail{ BelongModuneName,TMap<FString,FAssetDetail>{} });
						}
						TMap<FString, FAssetDetail>& CurrentModuleAssetDetails = OutAddAsset.mDependencies.Find(BelongModuneName)->mDependAssetDetails;
						CurrentModuleAssetDetails.Add(NewAssetItem, *NewVersionAssetModuleDetail.Find(NewAssetItem));
					}
				}
			}	
		}

		// Parser Modify Asset
		for (const auto& BaseVersionAssetModule : InBaseAssetModuleKeysList)
		{
			const FAssetDependenciesDetail& BaseVersionModuleAssetsDetail = *InBaseVersion.mDependencies.Find(BaseVersionAssetModule);
			const FAssetDependenciesDetail& NewVersionModuleAssetsDetail = *InNewVersion.mDependencies.Find(BaseVersionAssetModule);


			if (InNewVersion.mDependencies.Contains(BaseVersionAssetModule))
			{
				TArray<FString> BeseVersionCurrentModuleAssetListKeys;
				BaseVersionModuleAssetsDetail.mDependAssetDetails.GetKeys(BeseVersionCurrentModuleAssetListKeys);

				for (const auto& AssetItem : BeseVersionCurrentModuleAssetListKeys)
				{
					const FAssetDetail& BaseVersionAssetDetail = *BaseVersionModuleAssetsDetail.mDependAssetDetails.Find(AssetItem);
					const FAssetDetail& NewVersionAssetDetail = *NewVersionModuleAssetsDetail.mDependAssetDetails.Find(AssetItem);
					if (!(NewVersionAssetDetail == BaseVersionAssetDetail))
					{
						if (!OutModifyAsset.mDependencies.Contains(BaseVersionAssetModule))
						{
							OutModifyAsset.mDependencies.Add(BaseVersionAssetModule, FAssetDependenciesDetail{ BaseVersionAssetModule,TMap<FString,FAssetDetail>{} });
						}
						OutModifyAsset.mDependencies.Find(BaseVersionAssetModule)->mDependAssetDetails.Add(AssetItem, NewVersionAssetDetail);
					}
				}
			}
		}

	}

	return true;
}
