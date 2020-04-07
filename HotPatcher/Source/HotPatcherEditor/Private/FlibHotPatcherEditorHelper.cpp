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


UExportPatchSettings* UFlibHotPatcherEditorHelper::DeserializePatchConfig(UExportPatchSettings* InNewSetting,const FString& InContent)
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
					// UE_LOG(LogTemp, Log, TEXT("Filter: %s"), *CurrentFilterPath.Path);
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
					UFlibPatchParserHelper::DeSerializeSpecifyAssetInfoToJsonObject(InSpecifyAsset->AsObject(), CurrentAsset);
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
			DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bEnableExternFilesDiff);
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
			
			// chunk
			{
				DESERIAL_BOOL_BY_NAME(InNewSetting, JsonObject, bEnableChunk);
				TArray<TSharedPtr<FJsonValue>> ChunkJsonValues;

				ChunkJsonValues = JsonObject->GetArrayField(TEXT("ChunkInfos"));
				InNewSetting->ChunkInfos.Empty();

				for (const auto& ChunkJsonValue : ChunkJsonValues)
				{
					FChunkInfo CurrentChunk;

					TSharedPtr<FJsonObject> ChunkJsonObjectValue = ChunkJsonValue->AsObject();
					UFlibPatchParserHelper::DeSerializeFChunkInfoFromJsonObject(ChunkJsonObjectValue, CurrentChunk);
					InNewSetting->ChunkInfos.Add(CurrentChunk);
				}
			}
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
						// UE_LOG(LogTemp, Log, TEXT("FOUND ENUM INDEX SUCCESS:%s"),*EnumName);
						int32 EnumValue = ETargetPlatformEnum->GetValueByIndex(EnumIndex);
						ETargetPlatform CurrentEnum = (ETargetPlatform)EnumValue;
						FinalTargetPlatforms.Add(CurrentEnum);
					}
					else
					{
						// UE_LOG(LogTemp, Log, TEXT("FOUND ENUM INDEX FAILD:%s"), *EnumName);
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

bool UFlibHotPatcherEditorHelper::SerializePatchConfigToJsonObject(const UExportPatchSettings*const InPatchSetting,TSharedPtr<FJsonObject>& OutJsonObject)
{
	if (!InPatchSetting) return false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	OutJsonObject->SetStringField(TEXT("VersionId"), InPatchSetting->GetVersionId());
	OutJsonObject->SetBoolField(TEXT("bByBaseVersion"), InPatchSetting->IsByBaseVersion());
	OutJsonObject->SetStringField(TEXT("BaseVersion"), InPatchSetting->GetBaseVersion());

	auto SerializeArrayLambda = [&OutJsonObject](const TArray<FString>& InArray, const FString& InJsonArrayName)
	{
		TArray<TSharedPtr<FJsonValue>> ArrayJsonValueList;
		for (const auto& ArrayItem : InArray)
		{
			ArrayJsonValueList.Add(MakeShareable(new FJsonValueString(ArrayItem)));
		}
		OutJsonObject->SetArrayField(InJsonArrayName, ArrayJsonValueList);
	};
	auto ConvDirPathsToStrings = [](const TArray<FDirectoryPath>& InDirPaths)->TArray<FString>
	{
		TArray<FString> Resault;
		for (const auto& Dir : InDirPaths)
		{
			Resault.Add(Dir.Path);
		}
		return Resault;
	};

	SerializeArrayLambda(ConvDirPathsToStrings(InPatchSetting->AssetIncludeFilters), TEXT("AssetIncludeFilters"));
	SerializeArrayLambda(ConvDirPathsToStrings(InPatchSetting->AssetIgnoreFilters), TEXT("AssetIgnoreFilters"));
	OutJsonObject->SetBoolField(TEXT("bIncludeHasRefAssetsOnly"), InPatchSetting->IsIncludeHasRefAssetsOnly());

	// serialize specify asset
	{
		TArray<TSharedPtr<FJsonValue>> SpecifyAssetJsonValueObjectList;
		for (const auto& SpecifyAsset : InPatchSetting->GetIncludeSpecifyAssets())
		{
			TSharedPtr<FJsonObject> CurrentAssetJsonObject;
			UFlibPatchParserHelper::SerializeSpecifyAssetInfoToJsonObject(SpecifyAsset, CurrentAssetJsonObject);
			SpecifyAssetJsonValueObjectList.Add(MakeShareable(new FJsonValueObject(CurrentAssetJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("IncludeSpecifyAssets"), SpecifyAssetJsonValueObjectList);
	}

	OutJsonObject->SetBoolField(TEXT("bIncludeAssetRegistry"), InPatchSetting->IsIncludeAssetRegistry());
	OutJsonObject->SetBoolField(TEXT("bIncludeGlobalShaderCache"), InPatchSetting->IsIncludeGlobalShaderCache());
	OutJsonObject->SetBoolField(TEXT("bIncludeShaderBytecode"), InPatchSetting->IsIncludeShaderBytecode());
	OutJsonObject->SetBoolField(TEXT("bIncludeEngineIni"), InPatchSetting->IsIncludeEngineIni());
	OutJsonObject->SetBoolField(TEXT("bIncludePluginIni"), InPatchSetting->IsIncludePluginIni());
	OutJsonObject->SetBoolField(TEXT("bIncludeProjectIni"), InPatchSetting->IsIncludeProjectIni());
	OutJsonObject->SetBoolField(TEXT("bEnableExternFilesDiff"), InPatchSetting->IsEnableExternFilesDiff());
	// serialize all add extern file to pak
	{
		TArray<TSharedPtr<FJsonValue>> AddExFilesJsonObjectList;
		for (const auto& ExFileInfo : InPatchSetting->GetAddExternFiles())
		{
			TSharedPtr<FJsonObject> CurrentFileJsonObject;
			UFlibPatchParserHelper::SerializeExAssetFileInfoToJsonObject(ExFileInfo, CurrentFileJsonObject);
			AddExFilesJsonObjectList.Add(MakeShareable(new FJsonValueObject(CurrentFileJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("AddExternFileToPak"), AddExFilesJsonObjectList);
	}
	// serialize all add extern directory to pak
	{
		TArray<TSharedPtr<FJsonValue>> AddExDirectoryJsonObjectList;
		for (const auto& ExDirectoryInfo : InPatchSetting->GetAddExternDirectory())
		{
			TSharedPtr<FJsonObject> CurrentDirJsonObject;
			UFlibPatchParserHelper::SerializeExDirectoryInfoToJsonObject(ExDirectoryInfo, CurrentDirJsonObject);
			AddExDirectoryJsonObjectList.Add(MakeShareable(new FJsonValueObject(CurrentDirJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("AddExternDirectoryToPak"), AddExDirectoryJsonObjectList);
	}

	// serialize pakversion
	{
		OutJsonObject->SetBoolField(TEXT("bIncludePakVersionFile"), InPatchSetting->IsIncludePakVersion());
		OutJsonObject->SetStringField(TEXT("PakVersionFileMountPoint"), InPatchSetting->GetPakVersionFileMountPoint());
	}

	// chunk
	{
		OutJsonObject->SetBoolField(TEXT("bEnableChunk"), InPatchSetting->IsEnableChunk());

		TArray<TSharedPtr<FJsonValue>> ChunkInfosJsonValueList;

		for (const auto& Chunk : InPatchSetting->GetChunkInfos())
		{
			TSharedPtr<FJsonObject> CurrentChunkJsonObject;
			UFlibPatchParserHelper::SerializeFChunkInfoToJsonObject(Chunk, CurrentChunkJsonObject);
			ChunkInfosJsonValueList.Add(MakeShareable(new FJsonValueObject(CurrentChunkJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("ChunkInfos"), ChunkInfosJsonValueList);
	}
	// serialize UnrealPakOptions
	SerializeArrayLambda(InPatchSetting->GetUnrealPakOptions(), TEXT("UnrealPakOptions"));

	// serialize platform list
	{
		TArray<FString> AllPlatforms;
		for (const auto &Platform : InPatchSetting->GetPakTargetPlatforms())
		{
			// save UnrealPak.exe command file
			FString PlatformName;
			{
				FString EnumName;
				UEnum* ETargetPlatformEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETargetPlatform"), true);
				ETargetPlatformEnum->GetNameByValue((int64)Platform).ToString().Split(TEXT("::"), &EnumName, &PlatformName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			}
			AllPlatforms.AddUnique(PlatformName);
		}
		SerializeArrayLambda(AllPlatforms, TEXT("PakTargetPlatforms"));
	}

	OutJsonObject->SetBoolField(TEXT("bSavePakList"), InPatchSetting->IsSavePakList());
	OutJsonObject->SetBoolField(TEXT("bSaveDiffAnalysis"), InPatchSetting->IsSaveDiffAnalysis());
	OutJsonObject->SetBoolField(TEXT("bSavePatchConfig"), InPatchSetting->IsSavePatchConfig());
	OutJsonObject->SetStringField(TEXT("SavePath"), InPatchSetting->GetSaveAbsPath());

	return true;
}

bool UFlibHotPatcherEditorHelper::SerializeReleaseConfigToJsonObject(const UExportReleaseSettings*const InReleaseSetting, TSharedPtr<FJsonObject>& OutJsonObject)
{
	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	OutJsonObject->SetStringField(TEXT("VersionId"), InReleaseSetting->GetVersionId());

	auto SerializeArrayLambda = [&OutJsonObject](const TArray<FString>& InArray, const FString& InJsonArrayName)
	{
		TArray<TSharedPtr<FJsonValue>> ArrayJsonValueList;
		for (const auto& ArrayItem : InArray)
		{
			ArrayJsonValueList.Add(MakeShareable(new FJsonValueString(ArrayItem)));
		}
		OutJsonObject->SetArrayField(InJsonArrayName, ArrayJsonValueList);
	};
	auto ConvDirPathsToStrings = [](const TArray<FDirectoryPath>& InDirPaths)->TArray<FString>
	{
		TArray<FString> Resault;
		for (const auto& Dir : InDirPaths)
		{
			Resault.Add(Dir.Path);
		}
		return Resault;
	};

	SerializeArrayLambda(ConvDirPathsToStrings(InReleaseSetting->AssetIncludeFilters), TEXT("AssetIncludeFilters"));
	SerializeArrayLambda(ConvDirPathsToStrings(InReleaseSetting->AssetIgnoreFilters), TEXT("AssetIgnoreFilters"));

	OutJsonObject->SetBoolField(TEXT("bIncludeHasRefAssetsOnly"), InReleaseSetting->IsIncludeHasRefAssetsOnly());

	// serialize specify asset
	{
		TArray<TSharedPtr<FJsonValue>> SpecifyAssetJsonValueObjectList;
		for (const auto& SpecifyAsset : InReleaseSetting->GetSpecifyAssets())
		{
			TSharedPtr<FJsonObject> CurrentAssetJsonObject;
			UFlibPatchParserHelper::SerializeSpecifyAssetInfoToJsonObject(SpecifyAsset, CurrentAssetJsonObject);
			SpecifyAssetJsonValueObjectList.Add(MakeShareable(new FJsonValueObject(CurrentAssetJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("IncludeSpecifyAssets"), SpecifyAssetJsonValueObjectList);
	}

	// serialize all add extern file to pak
	{
		TArray<TSharedPtr<FJsonValue>> AddExFilesJsonObjectList;
		for (const auto& ExFileInfo : InReleaseSetting->GetAddExternFiles())
		{
			TSharedPtr<FJsonObject> CurrentFileJsonObject;
			UFlibPatchParserHelper::SerializeExAssetFileInfoToJsonObject(ExFileInfo, CurrentFileJsonObject);
			AddExFilesJsonObjectList.Add(MakeShareable(new FJsonValueObject(CurrentFileJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("AddExternFileToPak"), AddExFilesJsonObjectList);
	}
	// serialize all add extern directory to pak
	{
		TArray<TSharedPtr<FJsonValue>> AddExDirectoryJsonObjectList;
		for (const auto& ExDirectoryInfo : InReleaseSetting->GetAddExternDirectory())
		{
			TSharedPtr<FJsonObject> CurrentDirJsonObject;
			UFlibPatchParserHelper::SerializeExDirectoryInfoToJsonObject(ExDirectoryInfo, CurrentDirJsonObject);
			AddExDirectoryJsonObjectList.Add(MakeShareable(new FJsonValueObject(CurrentDirJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("AddExternDirectoryToPak"), AddExDirectoryJsonObjectList);
	}

	OutJsonObject->SetBoolField(TEXT("bSaveReleaseConfig"), InReleaseSetting->IsSaveConfig());
	OutJsonObject->SetStringField(TEXT("SavePath"), InReleaseSetting->GetSavePath());

	return true;
}

class UExportReleaseSettings* UFlibHotPatcherEditorHelper::DeserializeReleaseConfig(class UExportReleaseSettings* InNewSetting, const FString& InContent)
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
					// UE_LOG(LogTemp, Log, TEXT("Filter: %s"), *CurrentFilterPath.Path);
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

			InNewSetting->bSaveReleaseConfig = JsonObject->GetBoolField(TEXT("bSaveReleaseConfig"));
			InNewSetting->SavePath.Path = JsonObject->GetStringField(TEXT("SavePath"));
		}
	}

#undef DESERIAL_BOOL_BY_NAME
#undef DESERIAL_STRING_BY_NAME
	return InNewSetting;

}

#include "Kismet/KismetSystemLibrary.h"

FChunkInfo UFlibHotPatcherEditorHelper::MakeChunkFromPatchSettings(const UExportPatchSettings* InPatchSetting)
{
	FChunkInfo Chunk;
	if (!(InPatchSetting && UKismetSystemLibrary::IsValid(InPatchSetting)))
	{
		return Chunk;
	}
	
	Chunk.ChunkName = InPatchSetting->VersionId;
	Chunk.bMonolithic = false;
	Chunk.bSavePakCommands = true;
	Chunk.AssetIncludeFilters = InPatchSetting->GetAssetIncludeFilters();
	Chunk.AssetIgnoreFilters = InPatchSetting->GetAssetIgnoreFilters();
	Chunk.bAnalysisFilterDependencies = true;
	Chunk.IncludeSpecifyAssets = InPatchSetting->GetIncludeSpecifyAssets();
	Chunk.AddExternDirectoryToPak = InPatchSetting->GetAddExternDirectory();
	Chunk.AddExternFileToPak = InPatchSetting->GetAddExternFiles();
	Chunk.InternalFiles.bIncludeAssetRegistry = InPatchSetting->IsIncludeAssetRegistry();
	Chunk.InternalFiles.bIncludeGlobalShaderCache = InPatchSetting->IsIncludeGlobalShaderCache();
	Chunk.InternalFiles.bIncludeShaderBytecode = InPatchSetting->IsIncludeGlobalShaderCache();
	Chunk.InternalFiles.bIncludeEngineIni = InPatchSetting->IsIncludeEngineIni();
	Chunk.InternalFiles.bIncludePluginIni = InPatchSetting->IsIncludePluginIni();
	Chunk.InternalFiles.bIncludeProjectIni = InPatchSetting->IsIncludeProjectIni();

	return Chunk;
}

