// Fill out your copyright notice in the Description page of Project Settings.

// project header
#include "FlibPatchParserHelper.h"
#include "FlibAssetManageHelperEx.h"
#include "Struct/AssetManager/FFileArrayDirectoryVisitor.hpp"
#include "FLibAssetManageHelperEx.h"

// engine header
#include "Misc/SecureHash.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "Engine/EngineTypes.h"
#include "Misc/Paths.h"

TArray<FString> UFlibPatchParserHelper::GetAvailableMaps(FString GameName, bool IncludeEngineMaps, bool IncludePluginMaps, bool Sorted)
{
	const FString WildCard = FString::Printf(TEXT("*%s"), *FPackageName::GetMapPackageExtension());
	TArray<FString> AllMaps;
	TMap<FString,FString> AllEnableModules;

	UFLibAssetManageHelperEx::GetAllEnabledModuleName(AllEnableModules);

	auto ScanMapsByModuleName = [WildCard,&AllMaps](const FString& InModuleBaseDir)
	{
		TArray<FString> OutMaps;
		IFileManager::Get().FindFilesRecursive(OutMaps, *FPaths::Combine(*InModuleBaseDir, TEXT("Content")), *WildCard, true, false);
		
		for (const auto& MapPath : OutMaps)
		{
			AllMaps.Add(FPaths::GetBaseFilename(MapPath));
		}
	};

	ScanMapsByModuleName(AllEnableModules[TEXT("Game")]);
	if (IncludeEngineMaps)
	{
		ScanMapsByModuleName(AllEnableModules[TEXT("Engine")]);
	}
	if (IncludePluginMaps)
	{
		TArray<FString> AllModuleNames;
		AllEnableModules.GetKeys(AllModuleNames);

		for (const auto& ModuleName : AllModuleNames)
		{
			if (!ModuleName.Equals(TEXT("Game")) && !ModuleName.Equals(TEXT("Engine")))
			{
				ScanMapsByModuleName(AllEnableModules[ModuleName]);
			}
		}
	}
	if (Sorted)
	{
		AllMaps.Sort();
	}

	return AllMaps;
}

FString UFlibPatchParserHelper::GetProjectName()
{
	return FApp::GetProjectName();
}

FString UFlibPatchParserHelper::GetUnrealPakBinary()
{
#if PLATFORM_WINDOWS
	return FPaths::Combine(
		FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
		TEXT("Binaries"),
#if PLATFORM_64BITS	
		TEXT("Win64"),
#else
		TEXT("Win32"),
#endif
		TEXT("UnrealPak.exe")
	);
#endif

#if PLATFORM_MAC
    return FPaths::Combine(
            FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
            TEXT("Binaries"),
            TEXT("Mac"),
            TEXT("UnrealPak")
    );
#endif

	return TEXT("");
}

FString UFlibPatchParserHelper::GetUE4CmdBinary()
{
#if PLATFORM_WINDOWS
	return FPaths::Combine(
		FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
		TEXT("Binaries"),
#if PLATFORM_64BITS	
		TEXT("Win64"),
#else
		TEXT("Win32"),
#endif
		TEXT("UE4Editor-Cmd.exe")
	);
#endif
#if PLATFORM_MAC
    return FPaths::Combine(
		FPaths::ConvertRelativePathToFull(FPaths::EngineDir()),
		TEXT("Binaries"),
		TEXT("Mac"),
		TEXT("UE4Editor-Cmd")
	);
#endif
	return TEXT("");
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
		RootJsonObject->SetBoolField(TEXT("bIncludeHasRefAssetsOnly"), InVersion.bIncludeHasRefAssetsOnly);

		// serialize specify assets
		{
			TArray<TSharedPtr<FJsonValue>> AllSpecifyJsonObj;
			for(const auto& SpeficyAsset: InVersion.IncludeSpecifyAssets)
			{
				TSharedPtr<FJsonObject> SpecifyJsonObj = MakeShareable(new FJsonObject);
				FString LongPackageName;
				bool bConvStatus = UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(SpeficyAsset.Asset.ToString(), LongPackageName);
				SpecifyJsonObj->SetStringField(TEXT("Asset"), bConvStatus?LongPackageName:SpeficyAsset.Asset.ToString());
				SpecifyJsonObj->SetBoolField(TEXT("bAnalysisAssetDependencies"), SpeficyAsset.bAnalysisAssetDependencies);
				AllSpecifyJsonObj.Add(MakeShareable(new FJsonValueObject(SpecifyJsonObj)));
			}
			RootJsonObject->SetArrayField(TEXT("IncludeSpecifyAssets"), AllSpecifyJsonObj);
		}

		TSharedPtr<FJsonObject> AssetInfoJsonObject = MakeShareable(new FJsonObject);
		bool bSerializeAssetInfoStatus = UFLibAssetManageHelperEx::SerializeAssetDependenciesToJsonObject(InVersion.AssetInfo, AssetInfoJsonObject, TArray<FString>{TEXT("Script")});

		RootJsonObject->SetObjectField(TEXT("AssetInfo"), AssetInfoJsonObject);

		// serialize all extern file
		{
			TArray<TSharedPtr<FJsonValue>> ExternalFilesJsonObJList;

			TArray<FString> ExternalFilesKeys;
			InVersion.ExternalFiles.GetKeys(ExternalFilesKeys);
			for (auto& ExFile : ExternalFilesKeys)
			{
				TSharedPtr<FJsonObject> CurrentFile;
				if (UFlibPatchParserHelper::SerializeExAssetFileInfoToJsonObject(*InVersion.ExternalFiles.Find(ExFile), CurrentFile))
				{
					ExternalFilesJsonObJList.Add(MakeShareable(new FJsonValueObject(CurrentFile)));
				}
			}
			RootJsonObject->SetArrayField(TEXT("ExternalFiles"), ExternalFilesJsonObJList);
		}
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

		// deserialize extern files
		{
			TArray<TSharedPtr<FJsonValue>> AllExternalFilesJsonObj;
			AllExternalFilesJsonObj = InJsonObject->GetArrayField(TEXT("ExternalFiles"));
			// UE_LOG(LogTemp, Warning, TEXT("External Files num %d"), AllExternalFilesJsonObj.Num());

			for (const auto& FileJsonObj : AllExternalFilesJsonObj)
			{
				FExternAssetFileInfo CurrentFile;
				CurrentFile.FilePath.FilePath = FileJsonObj->AsObject()->GetStringField(TEXT("FilePath"));
				CurrentFile.MountPath = FileJsonObj->AsObject()->GetStringField(TEXT("MountPath"));
				CurrentFile.FileHash = FileJsonObj->AsObject()->GetStringField(TEXT("MD5Hash"));

				//UE_LOG(LogTemp, Warning, TEXT("External Files filepath %s"), *CurrentFile.FilePath.FilePath);
				//UE_LOG(LogTemp, Warning, TEXT("External Files filehash %s"), *CurrentFile.FileHash);
				//UE_LOG(LogTemp, Warning, TEXT("External Files mountpath %s"), *CurrentFile.MountPath);

				OutVersion.ExternalFiles.Add(CurrentFile.MountPath,CurrentFile);
			}
		}
		bRunStatus = true;
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::DiffVersionAssets(
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
					const FAssetDetail* BaseVersionAssetDetail = BaseVersionModuleAssetsDetail.mDependAssetDetails.Find(AssetItem);
					const FAssetDetail* NewVersionAssetDetail = NewVersionModuleAssetsDetail.mDependAssetDetails.Find(AssetItem);
					if (!NewVersionAssetDetail)
					{
						if (!OutDeleteAsset.mDependencies.Contains(BaseVersionAssetModule))
						{
							OutDeleteAsset.mDependencies.Add(BaseVersionAssetModule, FAssetDependenciesDetail{ BaseVersionAssetModule,TMap<FString,FAssetDetail>{} });
						}
						OutDeleteAsset.mDependencies.Find(BaseVersionAssetModule)->mDependAssetDetails.Add(AssetItem, *BaseVersionAssetDetail);
						continue;
					}

					if (!(*NewVersionAssetDetail == *BaseVersionAssetDetail))
					{
						if (!OutModifyAsset.mDependencies.Contains(BaseVersionAssetModule))
						{
							OutModifyAsset.mDependencies.Add(BaseVersionAssetModule, FAssetDependenciesDetail{ BaseVersionAssetModule,TMap<FString,FAssetDetail>{} });
						}
						OutModifyAsset.mDependencies.Find(BaseVersionAssetModule)->mDependAssetDetails.Add(AssetItem, *NewVersionAssetDetail);
					}
				}
			}
		}

	}

	return true;
}

bool UFlibPatchParserHelper::DiffVersionExFiles(
	const FHotPatcherVersion& InNewVersion,
	const FHotPatcherVersion& InBaseVersion,
	TArray<FExternAssetFileInfo>& OutAddFiles,
	TArray<FExternAssetFileInfo>& OutModifyFiles,
	TArray<FExternAssetFileInfo>& OutDeleteFiles
)
{
	OutAddFiles.Empty();
	OutModifyFiles.Empty();
	OutDeleteFiles.Empty();

	auto ParserAddFiles = [](const FHotPatcherVersion& lInNewVersion, const FHotPatcherVersion& lInBaseVersion, TArray<FExternAssetFileInfo>& lOutAddFiles)
	{
		TArray<FString> NewVersionFilesKeys;
		lInNewVersion.ExternalFiles.GetKeys(NewVersionFilesKeys);

		TArray<FString> BaseVersionFilesKeys;
		lInBaseVersion.ExternalFiles.GetKeys(BaseVersionFilesKeys);

		for (const auto& NewVersionFile : NewVersionFilesKeys)
		{
			if (!lInBaseVersion.ExternalFiles.Contains(NewVersionFile))
			{
				lOutAddFiles.Add(*lInNewVersion.ExternalFiles.Find(NewVersionFile));
			}
		}
	};

	// Parser Add Files
	ParserAddFiles(InNewVersion, InBaseVersion, OutAddFiles);
	// Parser delete Files
	ParserAddFiles(InBaseVersion, InNewVersion, OutDeleteFiles);

	// Parser modify Files
	{
		TArray<FString> NewVersionFilesKeys;
		InNewVersion.ExternalFiles.GetKeys(NewVersionFilesKeys);

		TArray<FString> BaseVersionFilesKeys;
		InBaseVersion.ExternalFiles.GetKeys(BaseVersionFilesKeys);

		for (const auto& NewVersionFile : NewVersionFilesKeys)
		{
			// UE_LOG(LogTemp, Log, TEXT("check file %s."), *NewVersionFile);
			if (InBaseVersion.ExternalFiles.Contains(NewVersionFile))
			{
				const FExternAssetFileInfo& NewFile = *InBaseVersion.ExternalFiles.Find(NewVersionFile);
				const FExternAssetFileInfo& BaseFile = *InNewVersion.ExternalFiles.Find(NewVersionFile);
				bool bIsSame = NewFile == BaseFile;
				if (!bIsSame)
				{
					// UE_LOG(LogTemp, Log, TEXT("%s is not same."), *NewFile.MountPath);
					OutModifyFiles.Add(*InNewVersion.ExternalFiles.Find(NewVersionFile));
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("base version not contains %s."), *NewVersionFile);
			}
		}
	}

	return true;
}
bool UFlibPatchParserHelper::SerializePakFileInfoToJsonString(const FPakFileInfo& InFileInfo, FString& OutJson)
{
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> JsonObject;
	if (UFlibPatchParserHelper::SerializePakFileInfoToJsonObject(InFileInfo, JsonObject))
	{
		bRunStatus = UFlibPatchParserHelper::SerializePakFileInfoFromJsonObjectToString(JsonObject, OutJson);
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializePakFileInfoFromJsonObjectToString(const TSharedPtr<FJsonObject>& InFileInfoJsonObject, FString& OutJson)
{
	bool bRunStatus = false;
	if (InFileInfoJsonObject.IsValid())
	{
		auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutJson);
		FJsonSerializer::Serialize(InFileInfoJsonObject.ToSharedRef(), JsonWriter);
		bRunStatus = true;
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializePakFileInfoToJsonObject(const FPakFileInfo& InFileInfo, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool RunStatus = false;
	
	if (!OutJsonObject.IsValid())
		OutJsonObject = MakeShareable(new FJsonObject);
	
	OutJsonObject->SetStringField(TEXT("File"), InFileInfo.FileName);
	OutJsonObject->SetStringField(TEXT("HASH"),InFileInfo.Hash);
	OutJsonObject->SetNumberField(TEXT("Size"), InFileInfo.FileSize);

	TSharedPtr<FJsonObject> PakVersionJsonObject = MakeShareable(new FJsonObject);
	if (UFlibPakHelper::SerializePakVersionToJsonObject(InFileInfo.PakVersion, PakVersionJsonObject))
	{
		OutJsonObject->SetObjectField(TEXT("PakVersion"),PakVersionJsonObject);
	}
	RunStatus = true;
	return RunStatus;
}

bool UFlibPatchParserHelper::SerializePakFileInfoListToJsonObject(const TArray<FPakFileInfo>& InFileInfoList, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	TSharedPtr<FJsonObject> RootJsonObj = MakeShareable(new FJsonObject);
	for (const auto& FileInfoItem : InFileInfoList)
	{
		TSharedPtr<FJsonObject> CurrentFileJsonObj = MakeShareable(new FJsonObject);
		if (UFlibPatchParserHelper::SerializePakFileInfoToJsonObject(FileInfoItem, CurrentFileJsonObj))
		{
			RootJsonObj->SetObjectField(*FileInfoItem.FileName, CurrentFileJsonObj);
		}
	}
	bRunStatus = true;
	OutJsonObject = RootJsonObj;
	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializePlatformPakInfoToString(const TMap<FString, FPakFileInfo>& InPakFilesMap, FString& OutString)
{
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> RootJsonObj = MakeShareable(new FJsonObject);

	bRunStatus = UFlibPatchParserHelper::SerializePlatformPakInfoToJsonObject(InPakFilesMap, RootJsonObj);

	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutString);
	FJsonSerializer::Serialize(RootJsonObj.ToSharedRef(), JsonWriter);

	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializePlatformPakInfoToJsonObject(const TMap<FString, FPakFileInfo>& InPakFilesMap, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;
	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}

	// serialize
	{
		TArray<FString> PakPlatformKeys;
		InPakFilesMap.GetKeys(PakPlatformKeys);

		for (const auto& PakPlatformKey : PakPlatformKeys)
		{
			TSharedPtr<FJsonObject> CurrentPlatformPakJsonObj;
			if (UFlibPatchParserHelper::SerializePakFileInfoToJsonObject(*InPakFilesMap.Find(PakPlatformKey), CurrentPlatformPakJsonObj))
			{
				OutJsonObject->SetObjectField(PakPlatformKey, CurrentPlatformPakJsonObj);
			}
		}
		bRunStatus = true;
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializeDiffAssetsInfomationToJsonObject(const FAssetDependenciesInfo& InAddAsset,
	const FAssetDependenciesInfo& InModifyAsset,
	const FAssetDependenciesInfo& InDeleteAsset,
	 TSharedPtr<FJsonObject>& OutJsonObject)
{
	if(!OutJsonObject.IsValid())
		OutJsonObject = MakeShareable(new FJsonObject);

	{
		// is empty Info
		auto IsEmptyInfo = [](const FAssetDependenciesInfo& InAssetInfo)->bool
		{
			TArray<FString> Keys;
			InAssetInfo.mDependencies.GetKeys(Keys);

			return Keys.Num() == 0;
		};

		auto SerializeAssetInfoToJson = [&OutJsonObject, &IsEmptyInfo](const FAssetDependenciesInfo& InAssetInfo, const FString& InDescrible)->bool
		{
			bool bRunStatus = false;
			if (!IsEmptyInfo(InAssetInfo))
			{
				TSharedPtr<FJsonObject> AssetsJsonObject = MakeShareable(new FJsonObject);
				bRunStatus = UFLibAssetManageHelperEx::SerializeAssetDependenciesToJsonObject(InAssetInfo, AssetsJsonObject, TArray<FString>{"Script"});
				if (bRunStatus)
				{
					OutJsonObject->SetObjectField(InDescrible, AssetsJsonObject);
				}
			}
			return bRunStatus;
		};

		SerializeAssetInfoToJson(InAddAsset, TEXT("AddedAssets"));
		SerializeAssetInfoToJson(InModifyAsset, TEXT("ModifiedAssets"));
		SerializeAssetInfoToJson(InDeleteAsset, TEXT("DeletedAssets"));
	}

	return !!InAddAsset.mDependencies.Num() || !!InModifyAsset.mDependencies.Num() || !!InDeleteAsset.mDependencies.Num();
}

FString UFlibPatchParserHelper::SerializeDiffAssetsInfomationToString(const FAssetDependenciesInfo& InAddAsset,
	const FAssetDependenciesInfo& InModifyAsset,
	const FAssetDependenciesInfo& InDeleteAsset)
{
	FString SerializeDiffInfo;
	TSharedPtr<FJsonObject> RootJsonObject;
	if (UFlibPatchParserHelper::SerializeDiffAssetsInfomationToJsonObject(InAddAsset, InModifyAsset, InDeleteAsset, RootJsonObject))
	{
		auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&SerializeDiffInfo);
		FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter);
	}
	
	return SerializeDiffInfo;
}

bool UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToJsonObject(
	const TArray<FExternAssetFileInfo>& InAddFiles,
	const TArray<FExternAssetFileInfo>& InModifyFiles,
	const TArray<FExternAssetFileInfo>& InDeleteFiles,
	TSharedPtr<FJsonObject>& OutJsonObject)
{
	if(!OutJsonObject.IsValid())
		OutJsonObject = MakeShareable(new FJsonObject);
	auto ParserFilesWithDescribleLambda = [&OutJsonObject](const TArray<FExternAssetFileInfo>& InFiles, const FString& InDescrible)
	{
		TArray<TSharedPtr<FJsonValue>> FileJsonValueList;
		for (const auto& File : InFiles)
		{
			FileJsonValueList.Add(MakeShareable(new FJsonValueString(File.MountPath)));
		}
		OutJsonObject->SetArrayField(InDescrible, FileJsonValueList);
	};

	ParserFilesWithDescribleLambda(InAddFiles, TEXT("AddFiles"));
	ParserFilesWithDescribleLambda(InModifyFiles, TEXT("ModifyFiles"));
	ParserFilesWithDescribleLambda(InDeleteFiles, TEXT("DeleteFiles"));

	return !!InAddFiles.Num() || !!InModifyFiles.Num() || !!InDeleteFiles.Num();
}

FString UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(
	const TArray<FExternAssetFileInfo>& InAddFiles,
	const TArray<FExternAssetFileInfo>& InModifyFiles,
	const TArray<FExternAssetFileInfo>& InDeleteFiles)
{
	FString SerializeDiffInfo;
	TSharedPtr<FJsonObject> RootJsonObject;
	if (UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToJsonObject(InAddFiles, InModifyFiles, InDeleteFiles,RootJsonObject))
	{
		auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&SerializeDiffInfo);
		FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter);
	}

	return SerializeDiffInfo;
}
bool UFlibPatchParserHelper::GetPakFileInfo(const FString& InFile, FPakFileInfo& OutFileInfo)
{
	bool bRunStatus = false;
	if (FPaths::FileExists(InFile))
	{
		FString PathPart;
		FString FileNamePart;
		FString ExtensionPart;

		FPaths::Split(InFile, PathPart, FileNamePart, ExtensionPart);

		FMD5Hash CurrentPakHash = FMD5Hash::HashFile(*InFile);
		OutFileInfo.FileName = FString::Printf(TEXT("%s.%s"),*FileNamePart,*ExtensionPart);
		OutFileInfo.Hash = LexToString(CurrentPakHash);
		OutFileInfo.FileSize = IFileManager::Get().FileSize(*InFile);
		bRunStatus = true;
	}
	return bRunStatus;
}

TArray<FString> UFlibPatchParserHelper::GetCookedGlobalShaderCacheFiles(const FString& InProjectDir, const FString& InPlatformName)
{
	TArray<FString> Resault;
	if (UFLibAssetManageHelperEx::IsValidPlatform(InPlatformName))
	{
		FString CookedEngineFolder = FPaths::Combine(InProjectDir,TEXT("Saved/Cooked"),InPlatformName,TEXT("Engine"));
		if (FPaths::DirectoryExists(CookedEngineFolder))
		{
			TArray<FString> FoundGlobalShaderCacheFiles;
			IFileManager::Get().FindFiles(FoundGlobalShaderCacheFiles, *CookedEngineFolder, TEXT("bin"));

			for (const auto& GlobalShaderCache : FoundGlobalShaderCacheFiles)
			{
				Resault.AddUnique(FPaths::Combine(CookedEngineFolder, GlobalShaderCache));
			}
		}
	}
	return Resault;
}


bool UFlibPatchParserHelper::GetCookedAssetRegistryFiles(const FString& InProjectAbsDir,const FString& InProjectName, const FString& InPlatformName, FString& OutFiles)
{
	bool bRunStatus = false;
	if (UFLibAssetManageHelperEx::IsValidPlatform(InPlatformName))
	{
		FString CookedPAssetRegistryFile = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"), InPlatformName, InProjectName,TEXT("AssetRegistry.bin"));
		if (FPaths::FileExists(CookedPAssetRegistryFile))
		{
			bRunStatus = true;
			OutFiles = CookedPAssetRegistryFile;
		}
	}

	return bRunStatus;
}

bool UFlibPatchParserHelper::GetCookedShaderBytecodeFiles(const FString& InProjectAbsDir, const FString& InProjectName, const FString& InPlatformName, bool InGalobalBytecode, bool InProjectBytecode, TArray<FString>& OutFiles)
{
	bool bRunStatus = false;
	OutFiles.Reset();

	if (UFLibAssetManageHelperEx::IsValidPlatform(InPlatformName))
	{
		FString CookedContentDir = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"), InPlatformName, InProjectName, TEXT("Content"));

		if (FPaths::DirectoryExists(CookedContentDir))
		{
			TArray<FString> ShaderbytecodeFiles;
			IFileManager::Get().FindFiles(ShaderbytecodeFiles, *CookedContentDir, TEXT("ushaderbytecode"));

			for (const auto& ShaderByteCodeFile : ShaderbytecodeFiles)
			{
				if (InGalobalBytecode && ShaderByteCodeFile.Contains(TEXT("Global")))
				{
					OutFiles.AddUnique(FPaths::Combine(CookedContentDir, ShaderByteCodeFile));
				}
				if (InProjectBytecode && ShaderByteCodeFile.Contains(InProjectName))
				{
					OutFiles.AddUnique(FPaths::Combine(CookedContentDir, ShaderByteCodeFile));
				}
				
			}

			bRunStatus = !!ShaderbytecodeFiles.Num();
		}
	}
	return bRunStatus;
}

TArray<FString> UFlibPatchParserHelper::GetProjectIniFiles(const FString& InProjectDir, const FString& InPlatformName)
{
	FString ConfigFolder = FPaths::Combine(InProjectDir, TEXT("Config"));
	return UFlibPatchParserHelper::GetIniConfigs(ConfigFolder,InPlatformName);
	
}

bool UFlibPatchParserHelper::ConvIniFilesToPakCommands(const FString& InEngineAbsDir,const FString& InProjectAbsDir,const FString& InProjectName, const TArray<FString>& InPakOptions, const TArray<FString>& InIniFiles, TArray<FString>& OutCommands)
{
	FString PakOptionsStr;
	{
		for (const auto& PakOption : InPakOptions)
		{
			PakOptionsStr += PakOption + TEXT(" ");
		}
	}
	OutCommands.Reset();
	bool bRunStatus = false;
	if (!FPaths::DirectoryExists(InProjectAbsDir) || !FPaths::DirectoryExists(InEngineAbsDir))
		return false;
	FString UProjectFile = FPaths::Combine(InProjectAbsDir, InProjectName + TEXT(".uproject"));
	if (!FPaths::FileExists(UProjectFile))
		return false;

	for (const auto& IniFile : InIniFiles)
	{
		bool bIsInProjectIni = false;
		if (IniFile.Contains(InProjectAbsDir))
		{
			bIsInProjectIni = true;
		}

		{
			FString IniAbsDir;
			FString IniFileName;
			FString IniExtention;
			FPaths::Split(IniFile, IniAbsDir, IniFileName, IniExtention);

			FString RelativePath;

			if (bIsInProjectIni)
			{
				RelativePath = FString::Printf(
					TEXT("../../../%s/"), 
					*InProjectName
				);
				
				FString RelativeToProjectDir = UKismetStringLibrary::GetSubstring(IniAbsDir, InProjectAbsDir.Len(), IniAbsDir.Len() - InProjectAbsDir.Len());
				RelativePath = FPaths::Combine(RelativePath, RelativeToProjectDir);
			}
			else
			{
				RelativePath = FString::Printf(
					TEXT("../../../Engine/")
				);
				FString RelativeToEngineDir = UKismetStringLibrary::GetSubstring(IniAbsDir, InEngineAbsDir.Len(), IniAbsDir.Len() - InEngineAbsDir.Len());
				RelativePath = FPaths::Combine(RelativePath, RelativeToEngineDir);
			}

			FString IniFileNameWithExten = FString::Printf(TEXT("%s.%s"),*IniFileName,*IniExtention);
			FString CookedIniRelativePath = FPaths::Combine(RelativePath, IniFileNameWithExten);
			FString CookCommand = FString::Printf(
				TEXT("\"%s\" \"%s\" %s"),
					*IniFile,
					*CookedIniRelativePath,
					*PakOptionsStr
			);
			OutCommands.AddUnique(CookCommand);
	
			bRunStatus = true;
		}
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::ConvNotAssetFileToPakCommand(const FString& InProjectDir, const FString& InPlatformName,const TArray<FString>& InPakOptions, const FString& InCookedFile, FString& OutCommand)
{
	FString PakOptionsStr;
	{
		for (const auto& PakOption : InPakOptions)
		{
			PakOptionsStr += PakOption + TEXT(" ");
		}
	}

	bool bRunStatus = false;
	if (FPaths::FileExists(InCookedFile))
	{
		FString CookPlatformAbsPath = FPaths::Combine(InProjectDir, TEXT("Saved/Cooked"), InPlatformName);

		FString RelativePath = UKismetStringLibrary::GetSubstring(InCookedFile, CookPlatformAbsPath.Len() + 1, InCookedFile.Len() - CookPlatformAbsPath.Len());
		FString AssetFileRelativeCookPath = FString::Printf(
			TEXT("../../../%s"),
			*RelativePath
		);

		OutCommand = FString::Printf(
			TEXT("\"%s\" \"%s\" %s"),
			*InCookedFile,
			*AssetFileRelativeCookPath,
			*PakOptionsStr
		);
		bRunStatus = true;
	}
	return bRunStatus;
}

FString UFlibPatchParserHelper::HashStringWithSHA1(const FString &InString)
{
	FSHAHash StringHash;
	FSHA1::HashBuffer(TCHAR_TO_ANSI(*InString), InString.Len(), StringHash.Hash);
	return StringHash.ToString();

}


TArray<FString> UFlibPatchParserHelper::GetIniConfigs(const FString& InSearchDir, const FString& InPlatformName)
{
	TArray<FString> Result;
	const FString SearchConfigAbsDir = FPaths::ConvertRelativePathToFull(InSearchDir);

	if (FPaths::DirectoryExists(SearchConfigAbsDir))
	{
		FFillArrayDirectoryVisitor Visitor;

		IFileManager::Get().IterateDirectory(*SearchConfigAbsDir, Visitor);

		for (const auto& IniFile : Visitor.Files)
		{
			if (!FPaths::GetCleanFilename(IniFile).Contains(TEXT("Editor")))
			{
				Result.Add(IniFile);
			}
		}
		int32 PlatformNameBeginIndex = SearchConfigAbsDir.Len() + 1;
		for (const auto& PlatformIniDirectory : Visitor.Directories)
		{
			FString DirectoryName = UKismetStringLibrary::GetSubstring(PlatformIniDirectory, PlatformNameBeginIndex, PlatformIniDirectory.Len() - PlatformNameBeginIndex);
			if (InPlatformName.Contains(DirectoryName))
			{
				FFillArrayDirectoryVisitor PlatformVisitor;

				IFileManager::Get().IterateDirectory(*PlatformIniDirectory, PlatformVisitor);

				for (const auto& PlatformIni : PlatformVisitor.Files)
				{
					Result.Add(PlatformIni);
				}
			}
		}
	}
	return Result;
}

TArray<FString> UFlibPatchParserHelper::GetEngineConfigs(const FString& InPlatformName)
{
	return UFlibPatchParserHelper::GetIniConfigs(FPaths::EngineConfigDir() , InPlatformName);
}

TArray<FString> UFlibPatchParserHelper::GetEnabledPluginConfigs(const FString& InPlatformName)
{
	TArray<FString> result;
	TArray<TSharedRef<IPlugin>> AllEnablePlugins = IPluginManager::Get().GetEnabledPlugins();

	for (const auto& Plugin : AllEnablePlugins)
	{
		FString PluginAbsPath;
		if (UFLibAssetManageHelperEx::GetPluginModuleAbsDir(Plugin->GetName(), PluginAbsPath))
		{
			FString PluginIniPath = FPaths::Combine(PluginAbsPath, TEXT("Config"));
			result.Append(UFlibPatchParserHelper::GetIniConfigs(PluginIniPath, InPlatformName));
			
		}
	}

	return result;
}


#include "Flib/FLibAssetManageHelperEx.h"
#include "Kismet/KismetStringLibrary.h"

TArray<FExternAssetFileInfo> UFlibPatchParserHelper::ParserExDirectoryAsExFiles(const TArray<FExternDirectoryInfo>& InExternDirectorys)
{
	TArray<FExternAssetFileInfo> result;

	if (!InExternDirectorys.Num())
		return result;

	for (const auto& DirectoryItem : InExternDirectorys)
	{
		FString DirAbsPath = FPaths::ConvertRelativePathToFull(DirectoryItem.DirectoryPath.Path);
		FPaths::MakeStandardFilename(DirAbsPath);
		if (!DirAbsPath.IsEmpty() && FPaths::DirectoryExists(DirAbsPath))
		{
			TArray<FString> DirectoryAllFiles;
			if (UFLibAssetManageHelperEx::FindFilesRecursive(DirAbsPath, DirectoryAllFiles, true))
			{
				int32 ParentDirectoryIndex = DirAbsPath.Len();
				for (const auto& File : DirectoryAllFiles)
				{
					FString RelativeParentPath = UKismetStringLibrary::GetSubstring(File, ParentDirectoryIndex, File.Len() - ParentDirectoryIndex);
					FString RelativeMountPointPath = FPaths::Combine(DirectoryItem.MountPoint, RelativeParentPath);
					FPaths::MakeStandardFilename(RelativeMountPointPath);

					FExternAssetFileInfo CurrentFile;
					CurrentFile.FilePath.FilePath = File;

					CurrentFile.MountPath = RelativeMountPointPath;
					if (!result.Contains(CurrentFile))
						result.Add(CurrentFile);

				}
			}
		}
	}
	return result;
}


TArray<FAssetDetail> UFlibPatchParserHelper::ParserExFilesInfoAsAssetDetailInfo(const TArray<FExternAssetFileInfo>& InExFiles)
{
	TArray<FAssetDetail> result;

	for (auto& File : InExFiles)
	{
		FAssetDetail CurrentFile;
		CurrentFile.mAssetType = TEXT("ExternalFile");
		CurrentFile.mPackagePath = File.MountPath;
		//CurrentFile.mGuid = File.GetFileHash();
		result.Add(CurrentFile);
	}

	return result;

}


TArray<FString> UFlibPatchParserHelper::GetIniFilesByPakInternalInfo(const FPakInternalInfo& InPakInternalInfo, const FString& PlatformName)
{
	TArray<FString> Inis;
	if (InPakInternalInfo.bIncludeEngineIni)
	{
		Inis.Append(UFlibPatchParserHelper::GetEngineConfigs(PlatformName));
	}
	if (InPakInternalInfo.bIncludePluginIni)
	{
		Inis.Append(UFlibPatchParserHelper::GetEnabledPluginConfigs(PlatformName));
	}
	if (InPakInternalInfo.bIncludeProjectIni)
	{
		Inis.Append(UFlibPatchParserHelper::GetProjectIniFiles(FPaths::ProjectDir(), PlatformName));
	}

	return Inis;
}


TArray<FString> UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(const FPakInternalInfo& InPakInternalInfo, const FString& PlatformName)
{
	TArray<FString> SearchAssetList;

	FString ProjectDir = FPaths::ProjectDir();
	FString ProjectName = UFlibPatchParserHelper::GetProjectName();

	if (InPakInternalInfo.bIncludeAssetRegistry)
	{
		FString AssetRegistryCookCommand;
		if (UFlibPatchParserHelper::GetCookedAssetRegistryFiles(ProjectDir, UFlibPatchParserHelper::GetProjectName(), PlatformName, AssetRegistryCookCommand))
		{
			SearchAssetList.AddUnique(AssetRegistryCookCommand);
		}
	}

	if (InPakInternalInfo.bIncludeGlobalShaderCache)
	{
		TArray<FString> GlobalShaderCacheList = UFlibPatchParserHelper::GetCookedGlobalShaderCacheFiles(ProjectDir, PlatformName);
		if (!!GlobalShaderCacheList.Num())
		{
			SearchAssetList.Append(GlobalShaderCacheList);
		}
	}

	if (InPakInternalInfo.bIncludeShaderBytecode)
	{
		TArray<FString> ShaderByteCodeFiles;

		if (UFlibPatchParserHelper::GetCookedShaderBytecodeFiles(ProjectDir, ProjectName, PlatformName, true, true, ShaderByteCodeFiles) &&
			!!ShaderByteCodeFiles.Num()
			)
		{
			SearchAssetList.Append(ShaderByteCodeFiles);
		}
	}
	return SearchAssetList;
}


TArray<FString> UFlibPatchParserHelper::GetPakCommandsFromInternalInfo(const FPakInternalInfo& InPakInternalInfo, const FString& PlatformName, const TArray<FString>& InPakOptions)
{
	TArray<FString> OutPakCommands;
	TArray<FString> AllNeedPakInternalCookedFiles;

	FString ProjectDir = FPaths::ProjectDir();
	AllNeedPakInternalCookedFiles.Append(UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(InPakInternalInfo, PlatformName));

	// combine as cook commands
	for (const auto& AssetFile : AllNeedPakInternalCookedFiles)
	{
		FString CurrentCommand;
		if (UFlibPatchParserHelper::ConvNotAssetFileToPakCommand(ProjectDir, PlatformName,InPakOptions, AssetFile, CurrentCommand))
		{
			OutPakCommands.AddUnique(CurrentCommand);
		}
	}

	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());

	auto CombineInPakCommands = [&OutPakCommands, &ProjectDir, &EngineAbsDir, &PlatformName,&InPakOptions](const TArray<FString>& IniFiles)
	{
		TArray<FString> result;

		TArray<FString> IniPakCommmands;
		UFlibPatchParserHelper::ConvIniFilesToPakCommands(
			EngineAbsDir,
			ProjectDir,
			UFlibPatchParserHelper::GetProjectName(),
			InPakOptions,
			IniFiles,
			IniPakCommmands
		);
		if (!!IniPakCommmands.Num())
		{
			OutPakCommands.Append(IniPakCommmands);
		}
	};

	TArray<FString> AllNeedPakIniFiles = UFlibPatchParserHelper::GetIniFilesByPakInternalInfo(InPakInternalInfo, PlatformName);
	CombineInPakCommands(AllNeedPakIniFiles);

	return OutPakCommands;
}

bool UFlibPatchParserHelper::SerializeExAssetFileInfoToJsonObject(const FExternAssetFileInfo& InExFileInfo, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	FString FileAbsPath = FPaths::ConvertRelativePathToFull(InExFileInfo.FilePath.FilePath);
	OutJsonObject->SetStringField(TEXT("FilePath"), FileAbsPath);
	OutJsonObject->SetStringField(TEXT("MD5Hash"), InExFileInfo.FileHash.IsEmpty()?InExFileInfo.GetFileHash(): InExFileInfo.FileHash);
	OutJsonObject->SetStringField(TEXT("MountPath"), InExFileInfo.MountPath);

	bRunStatus = true;

	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializeExDirectoryInfoToJsonObject(const FExternDirectoryInfo& InExDirectoryInfo, TSharedPtr<FJsonObject>& OutJsonObject)
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

bool UFlibPatchParserHelper::SerializeSpecifyAssetInfoToJsonObject(const FPatcherSpecifyAsset& InSpecifyAsset, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	bRunStatus = InSpecifyAsset.Asset.IsValid();

	if (bRunStatus)
	{
		OutJsonObject->SetStringField(TEXT("Asset"), InSpecifyAsset.Asset.GetAssetPathName().ToString());
		OutJsonObject->SetBoolField(TEXT("bAnalysisAssetDependencies"), InSpecifyAsset.bAnalysisAssetDependencies);
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::DeSerializeSpecifyAssetInfoToJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FPatcherSpecifyAsset& OutSpecifyAsset)
{
	bool bRunStatus = false;

	if (InJsonObject.IsValid())
	{
		OutSpecifyAsset.Asset = InJsonObject->GetStringField(TEXT("Asset"));
		OutSpecifyAsset.bAnalysisAssetDependencies = InJsonObject->GetBoolField(TEXT("bAnalysisAssetDependencies"));
		bRunStatus = true;
	}
	return bRunStatus;
}


bool UFlibPatchParserHelper::SerializeFPakInternalInfoToJsonObject(const FPakInternalInfo& InFPakInternalInfo, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	OutJsonObject->SetBoolField(TEXT("bIncludeAssetRegistry"), InFPakInternalInfo.bIncludeAssetRegistry);
	OutJsonObject->SetBoolField(TEXT("bIncludeGlobalShaderCache"), InFPakInternalInfo.bIncludeGlobalShaderCache);
	OutJsonObject->SetBoolField(TEXT("bIncludeShaderBytecode"), InFPakInternalInfo.bIncludeShaderBytecode);
	OutJsonObject->SetBoolField(TEXT("bIncludeEngineIni"), InFPakInternalInfo.bIncludeEngineIni);
	OutJsonObject->SetBoolField(TEXT("bIncludePluginIni"), InFPakInternalInfo.bIncludePluginIni);
	OutJsonObject->SetBoolField(TEXT("bIncludeProjectIni"), InFPakInternalInfo.bIncludeProjectIni);
	return bRunStatus;
}

bool UFlibPatchParserHelper::DeSerializeFPakInternalInfoFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FPakInternalInfo& OutFPakInternalInfo)
{
	OutFPakInternalInfo.bIncludeAssetRegistry = JsonObject->GetBoolField(TEXT("bIncludeAssetRegistry"));
	OutFPakInternalInfo.bIncludeGlobalShaderCache = JsonObject->GetBoolField(TEXT("bIncludeGlobalShaderCache"));
	OutFPakInternalInfo.bIncludeShaderBytecode = JsonObject->GetBoolField(TEXT("bIncludeShaderBytecode"));
	OutFPakInternalInfo.bIncludeEngineIni = JsonObject->GetBoolField(TEXT("bIncludeEngineIni"));
	OutFPakInternalInfo.bIncludePluginIni = JsonObject->GetBoolField(TEXT("bIncludePluginIni"));
	OutFPakInternalInfo.bIncludeProjectIni = JsonObject->GetBoolField(TEXT("bIncludeProjectIni"));
	return true;
}
bool UFlibPatchParserHelper::SerializeFChunkInfoToJsonObject(const FChunkInfo& InChunkInfo, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;

	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	OutJsonObject->SetStringField(TEXT("ChunkName"), InChunkInfo.ChunkName);
	OutJsonObject->SetBoolField(TEXT("bMonolithic"), InChunkInfo.bMonolithic);
	OutJsonObject->SetBoolField(TEXT("bSavePakCommands"), InChunkInfo.bSavePakCommands);
	auto ConvDirPathsToStrings = [](const TArray<FDirectoryPath>& InDirPaths)->TArray<FString>
	{
		TArray<FString> Resault;
		for (const auto& Dir : InDirPaths)
		{
			Resault.Add(Dir.Path);
		}
		return Resault;
	};

	auto SerializeArrayLambda = [&OutJsonObject](const TArray<FString>& InArray, const FString& InJsonArrayName)
	{
		TArray<TSharedPtr<FJsonValue>> ArrayJsonValueList;
		for (const auto& ArrayItem : InArray)
		{
			ArrayJsonValueList.Add(MakeShareable(new FJsonValueString(ArrayItem)));
		}
		OutJsonObject->SetArrayField(InJsonArrayName, ArrayJsonValueList);
	};
	SerializeArrayLambda(ConvDirPathsToStrings(InChunkInfo.AssetIncludeFilters), TEXT("AssetIncludeFilters"));

	// serialize specify asset
	{
		TArray<TSharedPtr<FJsonValue>> SpecifyAssetJsonValueObjectList;
		for (const auto& SpecifyAsset : InChunkInfo.IncludeSpecifyAssets)
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
		for (const auto& ExFileInfo : InChunkInfo.AddExternFileToPak)
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
		for (const auto& ExDirectoryInfo : InChunkInfo.AddExternDirectoryToPak)
		{
			TSharedPtr<FJsonObject> CurrentDirJsonObject;
			UFlibPatchParserHelper::SerializeExDirectoryInfoToJsonObject(ExDirectoryInfo, CurrentDirJsonObject);
			AddExDirectoryJsonObjectList.Add(MakeShareable(new FJsonValueObject(CurrentDirJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("AddExternDirectoryToPak"), AddExDirectoryJsonObjectList);
	}

	TSharedPtr<FJsonObject> OutInternalFilesJsonObject;
	UFlibPatchParserHelper::SerializeFPakInternalInfoToJsonObject(InChunkInfo.InternalFiles, OutInternalFilesJsonObject);
	OutJsonObject->SetObjectField(TEXT("InternalFiles"), OutInternalFilesJsonObject);
	return bRunStatus;
}


bool UFlibPatchParserHelper::DeSerializeFChunkInfoFromJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FChunkInfo& OutChunkInfo)
{
	OutChunkInfo.ChunkName = InJsonObject->GetStringField(TEXT("ChunkName"));
	OutChunkInfo.bMonolithic = InJsonObject->GetBoolField(TEXT("bMonolithic"));
	OutChunkInfo.bSavePakCommands = InJsonObject->GetBoolField(TEXT("bSavePakCommands"));
	auto ParserAssetFilter = [InJsonObject](const FString& InFilterName) -> TArray<FDirectoryPath>
	{
		TArray<TSharedPtr<FJsonValue>> FilterArray = InJsonObject->GetArrayField(InFilterName);

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

	OutChunkInfo.AssetIncludeFilters = ParserAssetFilter(TEXT("AssetIncludeFilters"));
	
	// PatcherSprcifyAsset
	{
		TArray<FPatcherSpecifyAsset> SpecifyAssets;
		TArray<TSharedPtr<FJsonValue>> SpecifyAssetsJsonObj = InJsonObject->GetArrayField(TEXT("IncludeSpecifyAssets"));

		for (const auto& InSpecifyAsset : SpecifyAssetsJsonObj)
		{
			FPatcherSpecifyAsset CurrentAsset;
			TSharedPtr<FJsonObject> SpecifyJsonObj = InSpecifyAsset->AsObject();
			CurrentAsset.Asset = SpecifyJsonObj->GetStringField(TEXT("Asset"));
			CurrentAsset.bAnalysisAssetDependencies = SpecifyJsonObj->GetBoolField(TEXT("bAnalysisAssetDependencies"));
			SpecifyAssets.Add(CurrentAsset);
		}
		OutChunkInfo.IncludeSpecifyAssets = SpecifyAssets;
	}

	// extern
	{
		// extern file 
		{
			TArray<FExternAssetFileInfo> AddExternFileToPak;

			TArray<TSharedPtr<FJsonValue>> ExternalFileJsonValues;

			ExternalFileJsonValues = InJsonObject->GetArrayField(TEXT("AddExternFileToPak"));

			for (const auto& FileJsonValue : ExternalFileJsonValues)
			{
				FExternAssetFileInfo CurrentExFile;
				TSharedPtr<FJsonObject> FileJsonObjectValue = FileJsonValue->AsObject();

				CurrentExFile.FilePath.FilePath = FileJsonObjectValue->GetStringField(TEXT("FilePath"));
				CurrentExFile.MountPath = FileJsonObjectValue->GetStringField(TEXT("MountPath"));

				AddExternFileToPak.Add(CurrentExFile);
			}
			OutChunkInfo.AddExternFileToPak = AddExternFileToPak;
		}
		// extern directory
		{
			TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
			TArray<TSharedPtr<FJsonValue>> ExternalDirJsonValues;

			ExternalDirJsonValues = InJsonObject->GetArrayField(TEXT("AddExternDirectoryToPak"));

			for (const auto& FileJsonValue : ExternalDirJsonValues)
			{
				FExternDirectoryInfo CurrentExDir;
				TSharedPtr<FJsonObject> FileJsonObjectValue = FileJsonValue->AsObject();

				CurrentExDir.DirectoryPath.Path = FileJsonObjectValue->GetStringField(TEXT("Directory"));
				CurrentExDir.MountPoint = FileJsonObjectValue->GetStringField(TEXT("MountPoint"));

				AddExternDirectoryToPak.Add(CurrentExDir);
			}
			OutChunkInfo.AddExternDirectoryToPak = AddExternDirectoryToPak;
		}

		// Internal Files

		UFlibPatchParserHelper::DeSerializeFPakInternalInfoFromJsonObject(InJsonObject->GetObjectField(TEXT("InternalFiles")), OutChunkInfo.InternalFiles);

	}

	return true;
}