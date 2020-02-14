// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibHotPatcherEditorHelper.h"
#include "CreatePatch/ExportPatchSettings.h"
#include "CreatePatch/ExportReleaseSettings.h"

// engine header
#include "Misc/SecureHash.h"

TArray<FString> UFlibHotPatcherEditorHelper::GetAllCookOption()
{
	TArray<FString> result
	{
		"Iterate",
		"UnVersioned",
		"CookAll",
		"Compressed"
	};
	return result;
}

void UFlibHotPatcherEditorHelper::CreateSaveFileNotify(const FText& InMsg, const FString& InSavedFile)
{
	auto Message = InMsg;
	FNotificationInfo Info(Message);
	Info.bFireAndForget = true;
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = false;
Info.bUseLargeFont = false;

const FString HyperLinkText = InSavedFile;
Info.Hyperlink = FSimpleDelegate::CreateStatic(
	[](FString SourceFilePath)
{
	FPlatformProcess::ExploreFolder(*SourceFilePath);
},
HyperLinkText
);
Info.HyperlinkText = FText::FromString(HyperLinkText);

FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
}

void UFlibHotPatcherEditorHelper::CheckInvalidCookFilesByAssetDependenciesInfo(
	const FString& InProjectAbsDir, 
	const FString& InPlatformName, 
	const FAssetDependenciesInfo& InAssetDependencies, 
	TArray<FAssetDetail>& OutValidAssets, 
	TArray<FAssetDetail>& OutInvalidAssets)
{
	OutValidAssets.Empty();
	OutInvalidAssets.Empty();
	TArray<FAssetDetail> AllAssetDetails;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(InAssetDependencies,AllAssetDetails);

	for (const auto& AssetDetail : AllAssetDetails)
	{
		TArray<FString> CookedAssetPath;
		TArray<FString> CookedAssetRelativePath;
		FString AssetLongPackageName;
		UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(AssetDetail.mPackagePath, AssetLongPackageName);
		if (UFLibAssetManageHelperEx::ConvLongPackageNameToCookedPath(
			InProjectAbsDir,
			InPlatformName,
			AssetLongPackageName,
			CookedAssetPath,
			CookedAssetRelativePath))
		{
			if (CookedAssetPath.Num() > 0)
			{
				OutValidAssets.Add(AssetDetail);
			}
			else
			{
				OutInvalidAssets.Add(AssetDetail);
			}
		}
	}
}

FHotPatcherVersion UFlibHotPatcherEditorHelper::ExportReleaseVersionInfo(
	const FString& InVersionId,
	const FString& InBaseVersion,
	const FString& InDate,
	const TArray<FString>& InIncludeFilter,
	const TArray<FString>& InIgnoreFilter,
	const TArray<FPatcherSpecifyAsset>& InIncludeSpecifyAsset,
	bool InIncludeHasRefAssetsOnly
)
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
	ExportVersion.bIncludeHasRefAssetsOnly = InIncludeHasRefAssetsOnly;
	ExportVersion.IncludeSpecifyAssets = InIncludeSpecifyAsset;

	TArray<FAssetDetail> FilterAssetList;
	{
		TArray<FAssetDetail> AllNeedPakRefAssets;
		{
			TArray<FAssetDetail> AllAssets;
			UFLibAssetManageHelperEx::GetAssetsList(ExportVersion.IncludeFilter, AllAssets);
			if (InIncludeHasRefAssetsOnly)
			{
				TArray<FAssetDetail> AllDontHasRefAssets;
				UFLibAssetManageHelperEx::FilterNoRefAssetsWithIgnoreFilter(AllAssets, ExportVersion.IgnoreFilter, AllNeedPakRefAssets, AllDontHasRefAssets);
			}
			else
			{
				AllNeedPakRefAssets = AllAssets;
			}
		}
		// 剔除ignore filter中指定的资源
		if (ExportVersion.IgnoreFilter.Num() > 0)
		{
			for (const auto& AssetDetail : AllNeedPakRefAssets)
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
					FilterAssetList.Add(AssetDetail);
				}
			}
		}
		else
		{
			FilterAssetList = AllNeedPakRefAssets;
		}

	}

	auto AnalysisAssetDependency = [](const TArray<FAssetDetail>& InAssetDetail,bool bInAnalysisDepend)->FAssetDependenciesInfo
	{
		FAssetDependenciesInfo RetAssetDepend;
		if (InAssetDetail.Num())
		{
			UFLibAssetManageHelperEx::CombineAssetsDetailAsFAssetDepenInfo(InAssetDetail, RetAssetDepend);

			if (bInAnalysisDepend)
			{
				FAssetDependenciesInfo AssetDependencies;
				UFLibAssetManageHelperEx::GetAssetListDependenciesForAssetDetail(InAssetDetail, AssetDependencies);

				RetAssetDepend = UFLibAssetManageHelperEx::CombineAssetDependencies(RetAssetDepend, AssetDependencies);
			}
		}
		return RetAssetDepend;
	};

	// 分析过滤器中指定的资源依赖
	FAssetDependenciesInfo FilterAssetDependencies = AnalysisAssetDependency(FilterAssetList,true);

	// Specify Assets
	FAssetDependenciesInfo SpecifyAssetDependencies;
	{
		for (const auto& SpecifyAsset : InIncludeSpecifyAsset)
		{
			FString AssetLongPackageName = SpecifyAsset.Asset.GetLongPackageName();
			FAssetDetail AssetDetail;
			if (UFLibAssetManageHelperEx::GetSpecifyAssetDetail(AssetLongPackageName, AssetDetail))
			{
				FAssetDependenciesInfo CurrentAssetDependencies = AnalysisAssetDependency(TArray<FAssetDetail>{AssetDetail}, SpecifyAsset.bAnalysisAssetDependencies);
				SpecifyAssetDependencies = UFLibAssetManageHelperEx::CombineAssetDependencies(SpecifyAssetDependencies, CurrentAssetDependencies);
			}
		}
	}

	ExportVersion.AssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(FilterAssetDependencies,SpecifyAssetDependencies);

	return ExportVersion;
}

bool UFlibHotPatcherEditorHelper::SerializeExAssetFileInfoToJsonObject(const FExternAssetFileInfo& InExFileInfo, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	FString FileAbsPath = FPaths::ConvertRelativePathToFull(InExFileInfo.FilePath.FilePath);
	FMD5Hash FileMD5Hash = FMD5Hash::HashFile(*FileAbsPath);
	OutJsonObject->SetStringField(TEXT("FilePath"), FileAbsPath);
	OutJsonObject->SetStringField(TEXT("MD5Hash"), LexToString(FileMD5Hash));
	OutJsonObject->SetStringField(TEXT("MountPath"), InExFileInfo.MountPath);

	bRunStatus = true;

	return bRunStatus;
}

bool UFlibHotPatcherEditorHelper::SerializeExDirectoryInfoToJsonObject(const FExternDirectoryInfo& InExDirectoryInfo, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	FString DirectoryAbsPath = FPaths::ConvertRelativePathToFull(InExDirectoryInfo.DirectoryPath.Path);
	OutJsonObject->SetStringField(TEXT("Directory"), DirectoryAbsPath);
	OutJsonObject->SetStringField(TEXT("MountPoint"), InExDirectoryInfo.MountPoint);
	bRunStatus = true;

	return bRunStatus;
}

bool UFlibHotPatcherEditorHelper::SerializeSpecifyAssetInfoToJsonObject(const FPatcherSpecifyAsset& InSpecifyAsset, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	OutJsonObject->SetStringField(TEXT("Asset"), InSpecifyAsset.Asset.GetLongPackageName());
	OutJsonObject->SetBoolField(TEXT("bAnalysisAssetDependencies"), InSpecifyAsset.bAnalysisAssetDependencies);
	bRunStatus = true;
	return bRunStatus;
}


TSharedPtr<UExportPatchSettings> UFlibHotPatcherEditorHelper::DeserializePatchConfig(TSharedPtr<UExportPatchSettings> InNewSetting,const FString& InContent)
{
#define DESERIAL_BOOL_BY_NAME(SettingObject,JsonObject,MemberName) SettingObject->MemberName = JsonObject->GetBoolField(TEXT(#MemberName));
#define DESERIAL_STRING_BY_NAME(SettingObject,JsonObject,MemberName) SettingObject->MemberName = JsonObject->GetStringField(TEXT(#MemberName));
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InContent);
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		// 
		{
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bByBaseVersion);
			InNewSetting->BaseVersion.FilePath = JsonObject->GetStringField(TEXT("BaseVersion"));
			DESERIAL_STRING_BY_NAME(InNewSetting, JsonObject, VersionId);


			auto ParserAssetFilter = [JsonObject](const FString& InFilterName) -> TArray<FDirectoryPath>
			{
				TArray<TSharedPtr<FJsonValue>> FilterArray = JsonObject->GetArrayField(InFilterName);

				TArray<FDirectoryPath> FilterResult;
				for (const auto& Filter : FilterArray)
				{
					FDirectoryPath CurrentFilterPath;
					CurrentFilterPath.Path = Filter->AsString();
					UE_LOG(LogTemp, Log, TEXT("Filter: %s"), *CurrentFilterPath.Path);
					FilterResult.Add(CurrentFilterPath);
				}
				return FilterResult;
			};

			InNewSetting->AssetIncludeFilters = ParserAssetFilter(TEXT("AssetIncludeFilters"));
			InNewSetting->AssetIgnoreFilters = ParserAssetFilter(TEXT("AssetIgnoreFilters"));

			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludeHasRefAssetsOnly);

			// PatcherSprcifyAsset
			{
				TArray<FPatcherSpecifyAsset> SpecifyAssets;
				TArray<TSharedPtr<FJsonValue>> SpecifyAssetsJsonObj = JsonObject->GetArrayField(TEXT("IncludeSpecifyAssets"));

				for (const auto& InSpecifyAsset : SpecifyAssetsJsonObj)
				{
					FPatcherSpecifyAsset CurrentAsset;
					TSharedPtr<FJsonObject> SpecifyJsonObj = InSpecifyAsset->AsObject();
					CurrentAsset.Asset = SpecifyJsonObj->GetStringField(TEXT("Asset"));
					CurrentAsset.bAnalysisAssetDependencies = SpecifyJsonObj->GetBoolField(TEXT("bAnalysisAssetDependencies"));
					SpecifyAssets.Add(CurrentAsset);
				}
				InNewSetting->IncludeSpecifyAssets = SpecifyAssets;
			}

			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludeAssetRegistry);
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludeGlobalShaderCache);
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludeShaderBytecode);
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludeEngineIni);
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludePluginIni);
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludeProjectIni);
			// extern
			{
				// extern file 
				{
					TArray<FExternAssetFileInfo> AddExternFileToPak;

					TArray<TSharedPtr<FJsonValue>> ExternalFileJsonValues;

					ExternalFileJsonValues = JsonObject->GetArrayField(TEXT("AddExternFileToPak"));

					for (const auto& FileJsonValue : ExternalFileJsonValues)
					{
						FExternAssetFileInfo CurrentExFile;
						TSharedPtr<FJsonObject> FileJsonObjectValue = FileJsonValue->AsObject();

						CurrentExFile.FilePath.FilePath = FileJsonObjectValue->GetStringField(TEXT("FilePath"));
						CurrentExFile.MountPath = FileJsonObjectValue->GetStringField(TEXT("MountPath"));

						AddExternFileToPak.Add(CurrentExFile);
					}
					InNewSetting->AddExternFileToPak = AddExternFileToPak;
				}
				// extern directory
				{
					TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
					TArray<TSharedPtr<FJsonValue>> ExternalDirJsonValues;

					ExternalDirJsonValues = JsonObject->GetArrayField(TEXT("AddExternDirectoryToPak"));

					for (const auto& FileJsonValue : ExternalDirJsonValues)
					{
						FExternDirectoryInfo CurrentExDir;
						TSharedPtr<FJsonObject> FileJsonObjectValue = FileJsonValue->AsObject();

						CurrentExDir.DirectoryPath.Path = FileJsonObjectValue->GetStringField(TEXT("Directory"));
						CurrentExDir.MountPoint = FileJsonObjectValue->GetStringField(TEXT("MountPoint"));

						AddExternDirectoryToPak.Add(CurrentExDir);
					}
					InNewSetting->AddExternDirectoryToPak = AddExternDirectoryToPak;
				}
			}
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludePakVersionFile);
			DESERIAL_STRING_BY_NAME(InNewSetting, JsonObject, PakVersionFileMountPoint);

			// UnrealPakOptions
			{
				
				TArray<TSharedPtr<FJsonValue>> UnrealPakOptionsJsonValues;
				UnrealPakOptionsJsonValues = JsonObject->GetArrayField(TEXT("UnrealPakOptions"));

				TArray<FString> FinalOptions;

				for (const auto& Option : UnrealPakOptionsJsonValues)
				{
					FinalOptions.Add(Option->AsString());
				}

				InNewSetting->UnrealPakOptions = FinalOptions;
			}
			
			// TargetPlatform
			{

				TArray<TSharedPtr<FJsonValue>> TargetPlatforms;
				TargetPlatforms = JsonObject->GetArrayField(TEXT("PakTargetPlatforms"));

				TArray<ETargetPlatform> FinalTargetPlatforms;
				
				for(const auto& Platform:TargetPlatforms)
				{
					FString EnumName = TEXT("ETargetPlatform::");
					EnumName.Append(Platform->AsString());

					UEnum* ETargetPlatformEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETargetPlatform"), true);
					
					int32 EnumIndex = ETargetPlatformEnum->GetIndexByName(FName(*EnumName));
					if (EnumIndex != INDEX_NONE)
					{
						UE_LOG(LogTemp, Log, TEXT("FOUND ENUM INDEX SUCCESS:%s"),*EnumName);
						int32 EnumValue = ETargetPlatformEnum->GetValueByIndex(EnumIndex);
						ETargetPlatform CurrentEnum = (ETargetPlatform)EnumValue;
						FinalTargetPlatforms.Add(CurrentEnum);
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("FOUND ENUM INDEX FAILD:%s"), *EnumName);
					}
				}
				InNewSetting->PakTargetPlatforms = FinalTargetPlatforms;
			}
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bSavePakList);
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bSaveDiffAnalysis);
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bSavePatchConfig);

			InNewSetting->SavePath.Path = JsonObject->GetStringField(TEXT("SavePath"));
		}
	}

#undef DESERIAL_BOOL_BY_NAME
#undef DESERIAL_STRING_BY_NAME
	return InNewSetting;
}

TSharedPtr<class UExportReleaseSettings> UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(TSharedPtr<class UExportReleaseSettings> InNewSetting, const FString& InContent)
{
#define DESERIAL_BOOL_BY_NAME(SettingObject,JsonObject,MemberName) SettingObject->MemberName = JsonObject->GetBoolField(TEXT(#MemberName));
#define DESERIAL_STRING_BY_NAME(SettingObject,JsonObject,MemberName) SettingObject->MemberName = JsonObject->GetStringField(TEXT(#MemberName));
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InContent);
	TSharedPtr<FJsonObject> JsonObject;
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		// 
		{
			DESERIAL_STRING_BY_NAME(InNewSetting, JsonObject, VersionId);


			auto ParserAssetFilter = [JsonObject](const FString& InFilterName) -> TArray<FDirectoryPath>
			{
				TArray<TSharedPtr<FJsonValue>> FilterArray = JsonObject->GetArrayField(InFilterName);

				TArray<FDirectoryPath> FilterResult;
				for (const auto& Filter : FilterArray)
				{
					FDirectoryPath CurrentFilterPath;
					CurrentFilterPath.Path = Filter->AsString();
					UE_LOG(LogTemp, Log, TEXT("Filter: %s"), *CurrentFilterPath.Path);
					FilterResult.Add(CurrentFilterPath);
				}
				return FilterResult;
			};

			InNewSetting->AssetIncludeFilters = ParserAssetFilter(TEXT("AssetIncludeFilters"));
			InNewSetting->AssetIgnoreFilters = ParserAssetFilter(TEXT("AssetIgnoreFilters"));

			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bIncludeHasRefAssetsOnly);

			// PatcherSprcifyAsset
			{
				TArray<FPatcherSpecifyAsset> SpecifyAssets;
				TArray<TSharedPtr<FJsonValue>> SpecifyAssetsJsonObj = JsonObject->GetArrayField(TEXT("IncludeSpecifyAssets"));

				for (const auto& InSpecifyAsset : SpecifyAssetsJsonObj)
				{
					FPatcherSpecifyAsset CurrentAsset;
					TSharedPtr<FJsonObject> SpecifyJsonObj = InSpecifyAsset->AsObject();
					CurrentAsset.Asset = SpecifyJsonObj->GetStringField(TEXT("Asset"));
					CurrentAsset.bAnalysisAssetDependencies = SpecifyJsonObj->GetBoolField(TEXT("bAnalysisAssetDependencies"));
					SpecifyAssets.Add(CurrentAsset);
				}
				InNewSetting->IncludeSpecifyAssets = SpecifyAssets;
			}

			InNewSetting->bSaveReleaseConfig = JsonObject->GetBoolField(TEXT("bSaveReleaseConfig"));
			InNewSetting->SavePath.Path = JsonObject->GetStringField(TEXT("SavePath"));
		}
	}

#undef DESERIAL_BOOL_BY_NAME
#undef DESERIAL_STRING_BY_NAME
	return InNewSetting;
}