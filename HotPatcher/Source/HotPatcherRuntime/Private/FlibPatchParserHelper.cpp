// Fill out your copyright notice in the Description page of Project Settings.

// project header
#include "FlibPatchParserHelper.h"
#include "FlibAssetManageHelperEx.h"
#include "Struct/AssetManager/FFileArrayDirectoryVisitor.hpp"
#include "FLibAssetManageHelperEx.h"
#include "Flib/FLibAssetManageHelperEx.h"

// engine header
#include "Misc/SecureHash.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "Engine/EngineTypes.h"
#include "Misc/Paths.h"
#include "Kismet/KismetStringLibrary.h"

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
				// UE_LOG(LogTemp, Log, TEXT("base version not contains %s."), *NewVersionFile);
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

	//TSharedPtr<FJsonObject> PakVersionJsonObject = MakeShareable(new FJsonObject);
	//if (UFlibPakHelper::SerializePakVersionToJsonObject(InFileInfo.PakVersion, PakVersionJsonObject))
	//{
	//	OutJsonObject->SetObjectField(TEXT("PakVersion"),PakVersionJsonObject);
	//}
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

bool UFlibPatchParserHelper::SerializePlatformPakInfoToString(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, FString& OutString)
{
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> RootJsonObj = MakeShareable(new FJsonObject);

	bRunStatus = UFlibPatchParserHelper::SerializePlatformPakInfoToJsonObject(InPakFilesMap, RootJsonObj);

	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutString);
	FJsonSerializer::Serialize(RootJsonObj.ToSharedRef(), JsonWriter);

	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializePlatformPakInfoToJsonObject(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, TSharedPtr<FJsonObject>& OutJsonObject)
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
			TArray<TSharedPtr<FJsonValue>> CurrentPlatformPaksObjectList;
			for (const auto& Pak : *InPakFilesMap.Find(PakPlatformKey))
			{
				TSharedPtr<FJsonObject> CurrentPakJsonObj;
				if (UFlibPatchParserHelper::SerializePakFileInfoToJsonObject(Pak, CurrentPakJsonObj))
				{
					CurrentPlatformPaksObjectList.Add(MakeShareable(new FJsonValueObject(CurrentPakJsonObj)));
				}
			}
			
			OutJsonObject->SetArrayField(PakPlatformKey, CurrentPlatformPaksObjectList);
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

bool UFlibPatchParserHelper::ConvIniFilesToPakCommands(
	const FString& InEngineAbsDir,
	const FString& InProjectAbsDir,
	const FString& InProjectName, 
	const TArray<FString>& InPakOptions, 
	const TArray<FString>& InIniFiles, 
	TArray<FString>& OutCommands,
	TFunction<void(const FPakCommand&)> InReceiveCommand
)
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
			FPakCommand CurrentPakCommand;
			CurrentPakCommand.PakCommands = TArray<FString>{ CookCommand };
			CurrentPakCommand.MountPath = CookedIniRelativePath;
			CurrentPakCommand.AssetPackage = UFlibPatchParserHelper::MountPathToRelativePath(CurrentPakCommand.MountPath);
			InReceiveCommand(CurrentPakCommand);
			OutCommands.AddUnique(CookCommand);
	
			bRunStatus = true;
		}
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::ConvNotAssetFileToPakCommand(
	const FString& InProjectDir, 
	const FString& InPlatformName,
	const TArray<FString>& InPakOptions, 
	const FString& InCookedFile, 
	FString& OutCommand,
	TFunction<void(const FPakCommand&)> InReceiveCommand
)
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
		FPakCommand CurrentPakCommand;
		CurrentPakCommand.PakCommands = TArray<FString>{ OutCommand };
		CurrentPakCommand.MountPath = AssetFileRelativeCookPath;
		CurrentPakCommand.AssetPackage = UFlibPatchParserHelper::MountPathToRelativePath(CurrentPakCommand.MountPath);
		InReceiveCommand(CurrentPakCommand);
		bRunStatus = true;
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::ConvNotAssetFileToExFile(const FString& InProjectDir, const FString& InPlatformName, const FString& InCookedFile, FExternAssetFileInfo& OutExFile)
{
	bool bRunStatus = false;
	if (FPaths::FileExists(InCookedFile))
	{
		FString CookPlatformAbsPath = FPaths::Combine(InProjectDir, TEXT("Saved/Cooked"), InPlatformName);

		FString RelativePath = UKismetStringLibrary::GetSubstring(InCookedFile, CookPlatformAbsPath.Len() + 1, InCookedFile.Len() - CookPlatformAbsPath.Len());
		FString AssetFileRelativeCookPath = FString::Printf(
			TEXT("../../../%s"),
			*RelativePath
		);

		OutExFile.FilePath.FilePath = InCookedFile;
		OutExFile.MountPath = AssetFileRelativeCookPath;
		OutExFile.GetFileHash();
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

	FString ProjectAbsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString ProjectName = UFlibPatchParserHelper::GetProjectName();

	if (InPakInternalInfo.bIncludeAssetRegistry)
	{
		FString AssetRegistryCookCommand;
		if (UFlibPatchParserHelper::GetCookedAssetRegistryFiles(ProjectAbsDir, UFlibPatchParserHelper::GetProjectName(), PlatformName, AssetRegistryCookCommand))
		{
			SearchAssetList.AddUnique(AssetRegistryCookCommand);
		}
	}

	if (InPakInternalInfo.bIncludeGlobalShaderCache)
	{
		TArray<FString> GlobalShaderCacheList = UFlibPatchParserHelper::GetCookedGlobalShaderCacheFiles(ProjectAbsDir, PlatformName);
		if (!!GlobalShaderCacheList.Num())
		{
			SearchAssetList.Append(GlobalShaderCacheList);
		}
	}

	if (InPakInternalInfo.bIncludeShaderBytecode)
	{
		TArray<FString> ShaderByteCodeFiles;

		if (UFlibPatchParserHelper::GetCookedShaderBytecodeFiles(ProjectAbsDir, ProjectName, PlatformName, true, true, ShaderByteCodeFiles) &&
			!!ShaderByteCodeFiles.Num()
			)
		{
			SearchAssetList.Append(ShaderByteCodeFiles);
		}
	}
	return SearchAssetList;
}


TArray<FExternAssetFileInfo> UFlibPatchParserHelper::GetInternalFilesAsExFiles(const FPakInternalInfo& InPakInternalInfo, const FString& InPlatformName)
{
	TArray<FExternAssetFileInfo> resultFiles;

	TArray<FString> AllNeedPakInternalCookedFiles;

	FString ProjectAbsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	AllNeedPakInternalCookedFiles.Append(UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(InPakInternalInfo, InPlatformName));

	for (const auto& File : AllNeedPakInternalCookedFiles)
	{
		FExternAssetFileInfo CurrentFile;
		UFlibPatchParserHelper::ConvNotAssetFileToExFile(ProjectAbsDir,InPlatformName, File, CurrentFile);
		resultFiles.Add(CurrentFile);
	}

	return resultFiles;
}

TArray<FString> UFlibPatchParserHelper::GetPakCommandsFromInternalInfo(
	const FPakInternalInfo& InPakInternalInfo, 
	const FString& PlatformName, 
	const TArray<FString>& InPakOptions, 
	TFunction<void(const FPakCommand&)> InReceiveCommand
)
{
	TArray<FString> OutPakCommands;
	TArray<FString> AllNeedPakInternalCookedFiles;

	FString ProjectAbsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	AllNeedPakInternalCookedFiles.Append(UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(InPakInternalInfo, PlatformName));

	// combine as cook commands
	for (const auto& AssetFile : AllNeedPakInternalCookedFiles)
	{
		FString CurrentCommand;
		if (UFlibPatchParserHelper::ConvNotAssetFileToPakCommand(ProjectAbsDir, PlatformName,InPakOptions, AssetFile, CurrentCommand, InReceiveCommand))
		{
			OutPakCommands.AddUnique(CurrentCommand);
		}
	}

	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());

	auto CombineInPakCommands = [&OutPakCommands, &ProjectAbsDir, &EngineAbsDir, &PlatformName,&InPakOptions,&InReceiveCommand](const TArray<FString>& IniFiles)
	{
		TArray<FString> result;

		TArray<FString> IniPakCommmands;
		UFlibPatchParserHelper::ConvIniFilesToPakCommands(
			EngineAbsDir,
			ProjectAbsDir,
			UFlibPatchParserHelper::GetProjectName(),
			InPakOptions,
			IniFiles,
			IniPakCommmands,
			InReceiveCommand
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


FChunkInfo UFlibPatchParserHelper::CombineChunkInfo(const FChunkInfo& R, const FChunkInfo& L)
{
	FChunkInfo result = R;

	result.ChunkName = FString::Printf(TEXT("%s_%s"), *R.ChunkName, *L.ChunkName);
	result.AssetIncludeFilters.Append(L.AssetIncludeFilters);
	result.AssetIgnoreFilters.Append(L.AssetIgnoreFilters);
	result.bAnalysisFilterDependencies = L.bAnalysisFilterDependencies || R.bAnalysisFilterDependencies;
	result.IncludeSpecifyAssets.Append(L.IncludeSpecifyAssets);
	result.AddExternDirectoryToPak.Append(L.AddExternDirectoryToPak);
	result.AddExternFileToPak.Append(L.AddExternFileToPak);

#define COMBINE_INTERNAL_FILES(InResult,Left,Right,PropertyName)\
	InResult.InternalFiles.PropertyName = Left.InternalFiles.PropertyName || Right.InternalFiles.PropertyName;

	COMBINE_INTERNAL_FILES(result, L, R, bIncludeAssetRegistry);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeGlobalShaderCache);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeShaderBytecode);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeEngineIni);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludePluginIni);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeProjectIni);

	return result;
}

FChunkInfo UFlibPatchParserHelper::CombineChunkInfos(const TArray<FChunkInfo>& Chunks)
{
	FChunkInfo result;
	
	for (const auto& Chunk : Chunks)
	{
		result = UFlibPatchParserHelper::CombineChunkInfo(result, Chunk);
	}
	return result;
}

TArray<FString> UFlibPatchParserHelper::GetDirectoryPaths(const TArray<FDirectoryPath>& InDirectoryPath)
{
	TArray<FString> Result;
	for (const auto& Filter : InDirectoryPath)
	{
		if (!Filter.Path.IsEmpty())
		{
			Result.AddUnique(Filter.Path);
		}
	}
	return Result;
}

TArray<FExternAssetFileInfo> UFlibPatchParserHelper::GetExternFilesFromChunk(const FChunkInfo& InChunk, bool bCalcHash)
{
	TArray<FExternAssetFileInfo> AllExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(InChunk.AddExternDirectoryToPak);

	for (auto& ExFile : InChunk.AddExternFileToPak)
	{
		if (!AllExternFiles.Contains(ExFile))
		{
			AllExternFiles.Add(ExFile);
		}
	}
	if (bCalcHash)
	{
		for (auto& ExFile : AllExternFiles)
		{
			ExFile.GenerateFileHash();
		}
	}
	return AllExternFiles;
}

FChunkAssetDescribe UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk)
{
	FChunkAssetDescribe ChunkAssetDescribe;
	// Collect Chunk Assets
	{

		auto GetAssetFilterPaths = [](const TArray<FDirectoryPath>& InFilters)->TArray<FString>
		{
			TArray<FString> Result;
			for (const auto& Filter : InFilters)
			{
				if (!Filter.Path.IsEmpty())
				{
					Result.AddUnique(Filter.Path);
				}
			}
			return Result;
		};
		FAssetDependenciesInfo SpecifyDependAssets;

		auto GetSpecifyAssets = [&SpecifyDependAssets](const TArray<FPatcherSpecifyAsset>& SpecifyAssets)->TArray<FString>
		{
			TArray<FString> result;
			for (const auto& Assset : SpecifyAssets)
			{
				FString CurrentAssetLongPackageName = Assset.Asset.GetLongPackageName();
				result.Add(CurrentAssetLongPackageName);
				if (Assset.bAnalysisAssetDependencies)
				{
					UFLibAssetManageHelperEx::GetAssetDependencies(CurrentAssetLongPackageName,Assset.AssetRegistryDependencyTypes, SpecifyDependAssets);
				}
			}
			return result;
		};

		TArray<FString> AssetFilterPaths = GetAssetFilterPaths(Chunk.AssetIncludeFilters);
		AssetFilterPaths.Append(GetSpecifyAssets(Chunk.IncludeSpecifyAssets));
		AssetFilterPaths.Append(UFLibAssetManageHelperEx::GetAssetLongPackageNameByAssetDependenciesInfo(SpecifyDependAssets));

		const FAssetDependenciesInfo& AddAssetsRef = DiffInfo.AddAssetDependInfo;
		const FAssetDependenciesInfo& ModifyAssetsRef = DiffInfo.ModifyAssetDependInfo;


		auto CollectChunkAssets = [](const FAssetDependenciesInfo& SearchBase, const TArray<FString>& SearchFilters)->FAssetDependenciesInfo
		{
			FAssetDependenciesInfo ResultAssetDependInfos;

			for (const auto& SearchItem : SearchFilters)
			{
				if (SearchItem.IsEmpty())
					continue;

				FString SearchModuleName;
				int32 findedPos = SearchItem.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1);
				if (findedPos != INDEX_NONE)
				{
					SearchModuleName = UKismetStringLibrary::GetSubstring(SearchItem, 1, findedPos - 1);
				}
				else
				{
					SearchModuleName = UKismetStringLibrary::GetSubstring(SearchItem, 1, SearchItem.Len() - 1);
				}

				if (!SearchModuleName.IsEmpty() && (SearchBase.mDependencies.Contains(SearchModuleName)))
				{
					if (!ResultAssetDependInfos.mDependencies.Contains(SearchModuleName))
						ResultAssetDependInfos.mDependencies.Add(SearchModuleName, FAssetDependenciesDetail(SearchModuleName, TMap<FString, FAssetDetail>{}));

					const FAssetDependenciesDetail& SearchBaseModule = *SearchBase.mDependencies.Find(SearchModuleName);

					TArray<FString> AllAssetKeys;
					SearchBaseModule.mDependAssetDetails.GetKeys(AllAssetKeys);

					for (const auto& KeyItem : AllAssetKeys)
					{
						if (KeyItem.StartsWith(SearchItem))
						{
							FAssetDetail FindedAsset = *SearchBaseModule.mDependAssetDetails.Find(KeyItem);
							if (!ResultAssetDependInfos.mDependencies.Find(SearchModuleName)->mDependAssetDetails.Contains(KeyItem))
							{
								ResultAssetDependInfos.mDependencies.Find(SearchModuleName)->mDependAssetDetails.Add(KeyItem, FindedAsset);
							}
						}
					}
				}
			}
			return ResultAssetDependInfos;
		};

		ChunkAssetDescribe.Assets = UFLibAssetManageHelperEx::CombineAssetDependencies(CollectChunkAssets(AddAssetsRef, AssetFilterPaths), CollectChunkAssets(ModifyAssetsRef, AssetFilterPaths));
	}

	// Collect Extern Files
	{
		TArray<FExternAssetFileInfo> AllFiles;

		TSet<FString> AllSearchFileFilter;
		{
			for (const auto& Directory : Chunk.AddExternDirectoryToPak)
			{
				if (!Directory.DirectoryPath.Path.IsEmpty())
					AllSearchFileFilter.Add(Directory.DirectoryPath.Path);
			}

			for (const auto& File : Chunk.AddExternFileToPak)
			{
				AllSearchFileFilter.Add(File.FilePath.FilePath);
			}
		}

		TArray<FExternAssetFileInfo> AddFilesRef = DiffInfo.AddExternalFiles;
		TArray<FExternAssetFileInfo> ModifyFilesRef = DiffInfo.ModifyExternalFiles;

		auto CollectExtenFilesLambda = [&AllFiles](const TArray<FExternAssetFileInfo>& SearchBase, const TSet<FString>& Filters)
		{
			for (const auto& Filter : Filters)
			{
				for (const auto& SearchItem : SearchBase)
				{
					if (SearchItem.FilePath.FilePath.StartsWith(Filter))
					{
						AllFiles.Add(SearchItem);
					}
				}
			}
		};
		CollectExtenFilesLambda(AddFilesRef, AllSearchFileFilter);
		CollectExtenFilesLambda(ModifyFilesRef, AllSearchFileFilter);

		ChunkAssetDescribe.AllExFiles.Append(AllFiles);
	}

	ChunkAssetDescribe.InternalFiles = Chunk.InternalFiles;

	return ChunkAssetDescribe;
}


TArray<FString> UFlibPatchParserHelper::CollectPakCommandsStringsByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName, const TArray<FString>& PakOptions)
{
	TArray<FString> ChunkPakCommands;
	{
		TArray<FPakCommand> ChunkPakCommands_r = UFlibPatchParserHelper::CollectPakCommandByChunk(DiffInfo, Chunk, PlatformName, PakOptions);
		for (const auto PakCommand : ChunkPakCommands_r)
		{
			ChunkPakCommands.Append(PakCommand.GetPakCommands());
		}
	}

	return ChunkPakCommands;
}

TArray<FPakCommand> UFlibPatchParserHelper::CollectPakCommandByChunk(const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName, const TArray<FString>& PakOptions)
{
	auto CollectPakCommandsByChunkLambda = [](const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName, const TArray<FString>& PakOptions)->TArray<FPakCommand>
	{
		FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(DiffInfo, Chunk);

		FString PakOptionsStr;
		{
			for (const auto& Param : PakOptions)
			{
				PakOptionsStr += TEXT(" ") + Param;
			}
		}

		TArray<FPakCommand> PakCommands;

		auto ReceivePakCommandAssetLambda = [&PakCommands](const TArray<FString>& InCommand, const FString& InMountPath,const FString& InLongPackageName)
		{
			FPakCommand CurrentPakCommand;
			CurrentPakCommand.PakCommands = InCommand;
			CurrentPakCommand.MountPath = InMountPath;
			CurrentPakCommand.AssetPackage = InLongPackageName;
			PakCommands.Add(CurrentPakCommand);
		};
		// Collect Chunk Assets
		{
			FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
			TArray<FString> AssetsPakCommands;
			UFLibAssetManageHelperEx::MakePakCommandFromAssetDependencies(ProjectDir, PlatformName, ChunkAssetsDescrible.Assets, PakOptions, AssetsPakCommands, ReceivePakCommandAssetLambda);
			
		}

		auto ReceivePakCommandExFilesLambda = [&PakCommands](const FPakCommand& InCommand)
		{
			PakCommands.Emplace(InCommand);
		};

		// Collect Extern Files
		{
			for (const auto& CollectFile : ChunkAssetsDescrible.AllExFiles)
			{
				FPakCommand CurrentPakCommand;
				CurrentPakCommand.MountPath = CollectFile.MountPath;
				CurrentPakCommand.AssetPackage = UFlibPatchParserHelper::MountPathToRelativePath(CurrentPakCommand.MountPath);
				CurrentPakCommand.PakCommands = TArray<FString>{ FString::Printf(TEXT("\"%s\" \"%s\"%s"), *CollectFile.FilePath.FilePath, *CollectFile.MountPath,*PakOptionsStr) };

				PakCommands.Add(CurrentPakCommand);
			}
		}
		// internal files
		UFlibPatchParserHelper::GetPakCommandsFromInternalInfo(ChunkAssetsDescrible.InternalFiles, PlatformName, PakOptions, ReceivePakCommandExFilesLambda);
		return PakCommands;
	};

	return CollectPakCommandsByChunkLambda(DiffInfo, Chunk, PlatformName, PakOptions);
}

FPatchVersionDiff UFlibPatchParserHelper::DiffPatchVersion(const FHotPatcherVersion& Base, const FHotPatcherVersion& New)
{
	FPatchVersionDiff VersionDiffInfo;
	FAssetDependenciesInfo BaseVersionAssetDependInfo = Base.AssetInfo;
	FAssetDependenciesInfo CurrentVersionAssetDependInfo = New.AssetInfo;

	UFlibPatchParserHelper::DiffVersionAssets(
		CurrentVersionAssetDependInfo,
		BaseVersionAssetDependInfo,
		VersionDiffInfo.AddAssetDependInfo,
		VersionDiffInfo.ModifyAssetDependInfo,
		VersionDiffInfo.DeleteAssetDependInfo
	);

	UFlibPatchParserHelper::DiffVersionExFiles(
		New,
		Base,
		VersionDiffInfo.AddExternalFiles,
		VersionDiffInfo.ModifyExternalFiles,
		VersionDiffInfo.DeleteExternalFiles
	);

	return VersionDiffInfo;
}

FHotPatcherVersion UFlibPatchParserHelper::ExportReleaseVersionInfo(
	const FString& InVersionId,
	const FString& InBaseVersion,
	const FString& InDate,
	const TArray<FString>& InIncludeFilter,
	const TArray<FString>& InIgnoreFilter,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	const TArray<FPatcherSpecifyAsset>& InIncludeSpecifyAsset,
	const TArray<FExternAssetFileInfo>& InAllExternFiles,
	bool InIncludeHasRefAssetsOnly,
	bool bInAnalysisFilterDepend
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
			UFLibAssetManageHelperEx::GetAssetsList(ExportVersion.IncludeFilter, AssetRegistryDependencyTypes, AllAssets);
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

	auto AnalysisAssetDependency = [](const TArray<FAssetDetail>& InAssetDetail, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes, bool bInAnalysisDepend)->FAssetDependenciesInfo
	{
		FAssetDependenciesInfo RetAssetDepend;
		if (InAssetDetail.Num())
		{
			UFLibAssetManageHelperEx::CombineAssetsDetailAsFAssetDepenInfo(InAssetDetail, RetAssetDepend);

			if (bInAnalysisDepend)
			{
				FAssetDependenciesInfo AssetDependencies;
				UFLibAssetManageHelperEx::GetAssetListDependenciesForAssetDetail(InAssetDetail,AssetRegistryDependencyTypes, AssetDependencies);

				RetAssetDepend = UFLibAssetManageHelperEx::CombineAssetDependencies(RetAssetDepend, AssetDependencies);
			}
		}
		return RetAssetDepend;
	};

	// 分析过滤器中指定的资源依赖
	FAssetDependenciesInfo FilterAssetDependencies = AnalysisAssetDependency(FilterAssetList, AssetRegistryDependencyTypes, bInAnalysisFilterDepend);

	// Specify Assets
	FAssetDependenciesInfo SpecifyAssetDependencies;
	{
		for (const auto& SpecifyAsset : InIncludeSpecifyAsset)
		{
			FString AssetLongPackageName = SpecifyAsset.Asset.GetLongPackageName();
			FAssetDetail AssetDetail;
			if (UFLibAssetManageHelperEx::GetSpecifyAssetDetail(AssetLongPackageName, AssetDetail))
			{
				FAssetDependenciesInfo CurrentAssetDependencies = AnalysisAssetDependency(TArray<FAssetDetail>{AssetDetail}, SpecifyAsset.AssetRegistryDependencyTypes, SpecifyAsset.bAnalysisAssetDependencies);
				SpecifyAssetDependencies = UFLibAssetManageHelperEx::CombineAssetDependencies(SpecifyAssetDependencies, CurrentAssetDependencies);
			}
		}
	}

	ExportVersion.AssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(FilterAssetDependencies, SpecifyAssetDependencies);

	for (const auto& File : InAllExternFiles)
	{
		if (!ExportVersion.ExternalFiles.Contains(File.FilePath.FilePath))
		{
			ExportVersion.ExternalFiles.Add(File.MountPath, File);
		}
	}
	return ExportVersion;
}


FHotPatcherVersion UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(const FString& InVersionId, const FString& InBaseVersion, const FString& InDate, const FChunkInfo& InChunkInfo, bool InIncludeHasRefAssetsOnly /*= false */,bool bInAnalysisFilterDepend /* = true*/)
{
	return UFlibPatchParserHelper::ExportReleaseVersionInfo(
		InVersionId,
		InBaseVersion,
		InDate,
		UFlibPatchParserHelper::GetDirectoryPaths(InChunkInfo.AssetIncludeFilters),
		UFlibPatchParserHelper::GetDirectoryPaths(InChunkInfo.AssetIgnoreFilters),
		InChunkInfo.AssetRegistryDependencyTypes,
		InChunkInfo.IncludeSpecifyAssets,
		UFlibPatchParserHelper::GetExternFilesFromChunk(InChunkInfo, true),
		InIncludeHasRefAssetsOnly,
		bInAnalysisFilterDepend
	);
}


FChunkAssetDescribe UFlibPatchParserHelper::DiffChunk(const FChunkInfo& CurrentVersionChunk, const FChunkInfo& TotalChunk, bool InIncludeHasRefAssetsOnly)
{
	FHotPatcherVersion TotalChunkVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT(""),
		TEXT(""),
		TEXT(""),
		TotalChunk,
		InIncludeHasRefAssetsOnly,
		TotalChunk.bAnalysisFilterDependencies
	);

	return UFlibPatchParserHelper::DiffChunkByBaseVersion(CurrentVersionChunk, TotalChunk ,TotalChunkVersion, InIncludeHasRefAssetsOnly);
}

FChunkAssetDescribe UFlibPatchParserHelper::DiffChunkByBaseVersion(const FChunkInfo& CurrentVersionChunk, const FChunkInfo& TotalChunk, const FHotPatcherVersion& BaseVersion, bool InIncludeHasRefAssetsOnly)
{
	FChunkAssetDescribe result;
	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT(""),
		TEXT(""),
		TEXT(""),
		CurrentVersionChunk,
		InIncludeHasRefAssetsOnly,
		CurrentVersionChunk.bAnalysisFilterDependencies
	);
	FPatchVersionDiff ChunkDiffInfo = DiffPatchVersion(BaseVersion, CurrentVersion);

	result.Assets = UFLibAssetManageHelperEx::CombineAssetDependencies(ChunkDiffInfo.AddAssetDependInfo, ChunkDiffInfo.ModifyAssetDependInfo);

	result.AllExFiles.Append(ChunkDiffInfo.AddExternalFiles);
	result.AllExFiles.Append(ChunkDiffInfo.ModifyExternalFiles);

	result.InternalFiles.bIncludeAssetRegistry = CurrentVersionChunk.InternalFiles.bIncludeAssetRegistry != TotalChunk.InternalFiles.bIncludeAssetRegistry;
	result.InternalFiles.bIncludeGlobalShaderCache = CurrentVersionChunk.InternalFiles.bIncludeGlobalShaderCache != TotalChunk.InternalFiles.bIncludeGlobalShaderCache;
	result.InternalFiles.bIncludeShaderBytecode = CurrentVersionChunk.InternalFiles.bIncludeShaderBytecode != TotalChunk.InternalFiles.bIncludeShaderBytecode;
	result.InternalFiles.bIncludeEngineIni = CurrentVersionChunk.InternalFiles.bIncludeEngineIni != TotalChunk.InternalFiles.bIncludeEngineIni;
	result.InternalFiles.bIncludePluginIni = CurrentVersionChunk.InternalFiles.bIncludePluginIni != TotalChunk.InternalFiles.bIncludePluginIni;
	result.InternalFiles.bIncludeProjectIni = CurrentVersionChunk.InternalFiles.bIncludeProjectIni != TotalChunk.InternalFiles.bIncludeProjectIni;

	return result;
}

TArray<FString> UFlibPatchParserHelper::GetPakCommandStrByCommands(const TArray<FPakCommand>& PakCommands,const TArray<FReplaceText>& InReplaceTexts)
{
	TArray<FString> ResultPakCommands;
	{
		for (const auto PakCommand : PakCommands)
		{
			const TArray<FString>& PakCommandOriginTexts = PakCommand.GetPakCommands();
			if (!!InReplaceTexts.Num())
			{
				for (const auto& PakCommandOriginText : PakCommandOriginTexts)
				{
					FString PakCommandTargetText = PakCommandOriginText;
					for (const auto& ReplaceText : InReplaceTexts)
					{
						ESearchCase::Type SearchCaseMode = ReplaceText.SearchCase == ESearchCaseMode::CaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
						PakCommandTargetText = PakCommandTargetText.Replace(*ReplaceText.From, *ReplaceText.To, SearchCaseMode);
					}
					ResultPakCommands.Add(PakCommandTargetText);
				}
			}
			else
			{
				ResultPakCommands.Append(PakCommandOriginTexts);
			}
		}
	}
	return ResultPakCommands;
}

FProcHandle UFlibPatchParserHelper::DoUnrealPak(TArray<FString> UnrealPakOptions, bool block)
{
	FString UnrealPakBinary = UFlibPatchParserHelper::GetUnrealPakBinary();

	FString CommandLine;
	for (const auto& Option : UnrealPakOptions)
	{
		CommandLine.Append(FString::Printf(TEXT(" %s"), *Option));
	}

	// create UnrealPak process

	uint32 *ProcessID = NULL;
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*UnrealPakBinary, *CommandLine, true, false, false, ProcessID, 0, NULL, NULL, NULL);

	if (ProcHandle.IsValid())
	{
		if (block)
		{
			FPlatformProcess::WaitForProc(ProcHandle);
		}
	}
	return ProcHandle;
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

		auto SerializeAssetDependencyTypes = [&OutJsonObject](const FString& InName, const TArray<EAssetRegistryDependencyTypeEx>& InTypes)
		{
			TArray<TSharedPtr<FJsonValue>> TypesJsonValues;
			for (const auto& Type : InTypes)
			{
				TSharedPtr<FJsonValue> CurrentJsonValue = MakeShareable(new FJsonValueString(UFlibPatchParserHelper::GetEnumNameByValue(Type)));
				TypesJsonValues.Add(CurrentJsonValue);
			}
			OutJsonObject->SetArrayField(InName, TypesJsonValues);
		};

		SerializeAssetDependencyTypes(TEXT("AssetRegistryDependencyTypes"), InSpecifyAsset.AssetRegistryDependencyTypes);
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
		// serialize AssetRegistryDependencyTypes
		{
			TArray<EAssetRegistryDependencyTypeEx> result;
			TArray<TSharedPtr<FJsonValue>> AssetRegistryDependencyTypes = InJsonObject->GetArrayField(TEXT("AssetRegistryDependencyTypes"));
			for (const auto& TypeJsonValue : AssetRegistryDependencyTypes)
			{
				EAssetRegistryDependencyTypeEx CurrentType;
				if (UFlibPatchParserHelper::GetEnumValueByName(TypeJsonValue->AsString(),CurrentType))
				{
					result.AddUnique(CurrentType);
				}
			}
			OutSpecifyAsset.AssetRegistryDependencyTypes = result;
		}

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
	OutJsonObject->SetStringField(TEXT("MonolithicPathMode"), UFlibPatchParserHelper::GetEnumNameByValue(InChunkInfo.MonolithicPathMode));
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

	auto SerializeAssetDependencyTypes = [&OutJsonObject](const FString& InName, const TArray<EAssetRegistryDependencyTypeEx>& InTypes)
	{
		TArray<TSharedPtr<FJsonValue>> TypesJsonValues;
		for (const auto& Type : InTypes)
		{
			TSharedPtr<FJsonValue> CurrentJsonValue = MakeShareable(new FJsonValueString(UFlibPatchParserHelper::GetEnumNameByValue(Type)));
			TypesJsonValues.Add(CurrentJsonValue);
		}
		OutJsonObject->SetArrayField(InName, TypesJsonValues);
	};
	OutJsonObject->SetBoolField(TEXT("bAnalysisFilterDependencies"), InChunkInfo.bAnalysisFilterDependencies);
	SerializeAssetDependencyTypes(TEXT("AssetRegistryDependencyTypes"), InChunkInfo.AssetRegistryDependencyTypes);

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
	UFlibPatchParserHelper::GetEnumValueByName(InJsonObject->GetStringField(TEXT("MonolithicPathMode")),OutChunkInfo.MonolithicPathMode);
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

	OutChunkInfo.bAnalysisFilterDependencies = InJsonObject->GetBoolField(TEXT("bAnalysisFilterDependencies"));
	// deserialize AssetRegistryDependencyTypes
	{
		TArray<EAssetRegistryDependencyTypeEx> result;
		TArray<TSharedPtr<FJsonValue>> AssetRegistryDependencyTypes = InJsonObject->GetArrayField(TEXT("AssetRegistryDependencyTypes"));
		for (const auto& TypeJsonValue : AssetRegistryDependencyTypes)
		{
			EAssetRegistryDependencyTypeEx CurrentType;
			if (UFlibPatchParserHelper::GetEnumValueByName(TypeJsonValue->AsString(),CurrentType))
			{
				result.AddUnique(CurrentType);
			}
		}
		OutChunkInfo.AssetRegistryDependencyTypes = result;
	}

	// PatcherSprcifyAsset
	{
		TArray<FPatcherSpecifyAsset> SpecifyAssets;
		TArray<TSharedPtr<FJsonValue>> SpecifyAssetsJsonObj = InJsonObject->GetArrayField(TEXT("IncludeSpecifyAssets"));

		for (const auto& InSpecifyAsset : SpecifyAssetsJsonObj)
		{
			FPatcherSpecifyAsset CurrentAsset;
			TSharedPtr<FJsonObject> SpecifyJsonObj = InSpecifyAsset->AsObject();
			if (UFlibPatchParserHelper::DeSerializeSpecifyAssetInfoToJsonObject(SpecifyJsonObj, CurrentAsset))
			{
				SpecifyAssets.Add(CurrentAsset);
			}
			
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

FAssetRelatedInfo UFlibPatchParserHelper::GetAssetRelatedInfo(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes)
{
	FAssetRelatedInfo Dependency;
	Dependency.Asset = InAsset;
	FString LongPackageName;
	if (UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(InAsset.mPackagePath, LongPackageName))
	{
		TArray<EAssetRegistryDependencyType::Type> SearchAssetDepTypes;
		for (const auto& Type : AssetRegistryDependencyTypes)
		{
			SearchAssetDepTypes.AddUnique(UFLibAssetManageHelperEx::ConvAssetRegistryDependencyToInternal(Type));
		}

		UFLibAssetManageHelperEx::GetAssetDependency(LongPackageName, AssetRegistryDependencyTypes,Dependency.AssetDependency, false);
		UFLibAssetManageHelperEx::GetAssetReference(InAsset, SearchAssetDepTypes, Dependency.AssetReference);

	}

	return Dependency;
}

TArray<FAssetRelatedInfo> UFlibPatchParserHelper::GetAssetsRelatedInfo(const TArray<FAssetDetail>& InAssets, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes)
{
	TArray<FAssetRelatedInfo> AssetsDependency;

	for (const auto& Asset : InAssets)
	{
		AssetsDependency.Add(UFlibPatchParserHelper::GetAssetRelatedInfo(Asset, AssetRegistryDependencyTypes));
	}
	return AssetsDependency;
}

TArray<FAssetRelatedInfo> UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(const FAssetDependenciesInfo& InAssetsDependencies, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes)
{
	TArray<FAssetDetail> AssetsDetail;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(InAssetsDependencies, AssetsDetail);
	return UFlibPatchParserHelper::GetAssetsRelatedInfo(AssetsDetail, AssetRegistryDependencyTypes);
}

bool UFlibPatchParserHelper::SerializeAssetRelatedInfoAsJsonObject(const FAssetRelatedInfo& InAssetDependency, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool RunStatus = false;

	if (!OutJsonObject.IsValid())
		OutJsonObject = MakeShareable(new FJsonObject);

	FString LongPackageName;
	UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(InAssetDependency.Asset.mPackagePath, LongPackageName);
	OutJsonObject->SetStringField(TEXT("Asset"), LongPackageName);

	

	auto SerializeAssets = [&OutJsonObject](const FString& InCategory, const TArray<FAssetDetail>& InAssets)
	{
		TArray<TSharedPtr<FJsonValue>> RefAssetsObjectList;
		for (const auto& RefAsset : InAssets)
		{
			TSharedPtr<FJsonObject> CurrentRefAssetJsonObject = UFLibAssetManageHelperEx::SerilizeAssetDetial(RefAsset);
			RefAssetsObjectList.Add(MakeShareable(new FJsonValueObject(CurrentRefAssetJsonObject)));
		}
		OutJsonObject->SetArrayField(InCategory, RefAssetsObjectList);
	};
	
	SerializeAssets(TEXT("Dependency"), InAssetDependency.AssetDependency);
	SerializeAssets(TEXT("Reference"), InAssetDependency.AssetReference);
	return true;
}

bool UFlibPatchParserHelper::SerializeAssetsRelatedInfoAsJsonObject(const TArray<FAssetRelatedInfo>& InAssetsDependency, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool RunStatus = false;

	if (!OutJsonObject.IsValid())
		OutJsonObject = MakeShareable(new FJsonObject);

	TArray<TSharedPtr<FJsonValue>> RefAssetsObjectList;
	for (const auto& AssetDependency: InAssetsDependency)
	{
		TSharedPtr<FJsonObject> CurrentAssetDependencyJsonObject;
		if(UFlibPatchParserHelper::SerializeAssetRelatedInfoAsJsonObject(AssetDependency,CurrentAssetDependencyJsonObject))
		RefAssetsObjectList.Add(MakeShareable(new FJsonValueObject(CurrentAssetDependencyJsonObject)));
	}
	OutJsonObject->SetArrayField(TEXT("AssetsRelatedInfos"),RefAssetsObjectList);
	return true;

}

bool UFlibPatchParserHelper::SerializeAssetsRelatedInfoAsString(const TArray<FAssetRelatedInfo>& InAssetsDependency, FString& OutString)
{
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> RootJsonObj = MakeShareable(new FJsonObject);

	bRunStatus = UFlibPatchParserHelper::SerializeAssetsRelatedInfoAsJsonObject(InAssetsDependency, RootJsonObj);

	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutString);
	FJsonSerializer::Serialize(RootJsonObj.ToSharedRef(), JsonWriter);

	return bRunStatus;
}

TArray<TSharedPtr<FJsonValue>> UFlibPatchParserHelper::SerializeFReplaceTextsAsJsonValues(const TArray<FReplaceText>& InReplaceTexts)
{
	TArray<TSharedPtr<FJsonValue>> ReplaceTextJsonValueList;

	for (const auto& ReplaceText : InReplaceTexts)
	{
		ReplaceTextJsonValueList.Add(MakeShareable(new FJsonValueObject(UFlibPatchParserHelper::SerializeFReplaceTextAsJsonObject(ReplaceText))));
	}
	return ReplaceTextJsonValueList;
}

TSharedPtr<FJsonObject> UFlibPatchParserHelper::SerializeFReplaceTextAsJsonObject(const FReplaceText& InReplaceText)
{
	TSharedPtr<FJsonObject> resultJsonObject = MakeShareable(new FJsonObject);
	resultJsonObject->SetStringField(TEXT("From"), InReplaceText.From);
	resultJsonObject->SetStringField(TEXT("To"), InReplaceText.To);
	resultJsonObject->SetStringField(TEXT("SearchCaseMode"), UFlibPatchParserHelper::GetEnumNameByValue(InReplaceText.SearchCase));
	return resultJsonObject;
}

FReplaceText UFlibPatchParserHelper::DeSerializeFReplaceText(const TSharedPtr<FJsonObject>& InReplaceTextJsonObject)
{
	FReplaceText result;
	if (InReplaceTextJsonObject.IsValid())
	{
		result.From = InReplaceTextJsonObject->GetStringField(TEXT("From"));
		result.To = InReplaceTextJsonObject->GetStringField(TEXT("To"));

		FString SearchCaseModeStr = InReplaceTextJsonObject->GetStringField(TEXT("SearchCaseMode"));
		UFlibPatchParserHelper::GetEnumValueByName(SearchCaseModeStr, result.SearchCase);

	}
	return result;
}


TSharedPtr<FJsonObject> UFlibPatchParserHelper::SerializeCookerConfigAsJsonObject(const FCookerConfig& InConfig)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("EngineBin"),InConfig.EngineBin);
	JsonObject->SetStringField(TEXT("ProjectPath"), InConfig.ProjectPath);
	JsonObject->SetStringField(TEXT("EngineParams"), InConfig.EngineParams);

	auto SerializeArrayLambda = [&JsonObject](const FString& InName, const TArray<FString>& InArray)
	{
		TArray<TSharedPtr<FJsonValue>> JsonValueList;

		for (const auto& Item : InArray)
		{
			JsonValueList.Add(MakeShareable(new FJsonValueString(Item)));
		}
		JsonObject->SetArrayField(InName, JsonValueList);
	};
	SerializeArrayLambda(TEXT("CookPlatforms"), InConfig.CookPlatforms);
	SerializeArrayLambda(TEXT("CookMaps"), InConfig.CookMaps);
	JsonObject->SetBoolField(TEXT("bCookAllMap"),InConfig.bCookAllMap);
	SerializeArrayLambda(TEXT("CookFilter"), InConfig.CookFilter);
	SerializeArrayLambda(TEXT("CookSettings"), InConfig.CookSettings);
	JsonObject->SetStringField(TEXT("Options"), InConfig.Options);

	return JsonObject;
}

FString UFlibPatchParserHelper::SerializeCookerConfigAsString(const TSharedPtr<FJsonObject>& InConfigJson)
{
	FString result;
	auto JsonWriter = TJsonWriterFactory<>::Create(&result);
	FJsonSerializer::Serialize(InConfigJson.ToSharedRef(), JsonWriter);
	return result;
}

FCookerConfig UFlibPatchParserHelper::DeSerializeCookerConfig(const FString& InJsonContent)
{
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InJsonContent);
	TSharedPtr<FJsonObject> JsonObject;
	FJsonSerializer::Deserialize(JsonReader, JsonObject);

	FCookerConfig result;
	result.EngineBin = JsonObject->GetStringField(TEXT("EngineBin"));
	result.ProjectPath = JsonObject->GetStringField(TEXT("ProjectPath"));
	result.EngineParams = JsonObject->GetStringField(TEXT("EngineParams"));

	auto DeserializeArray = [&JsonObject](const FString& InName)->TArray<FString>
	{
		TArray<FString> result;
		TArray<TSharedPtr<FJsonValue>> JsonValueList = JsonObject->GetArrayField(InName);

		for (const auto& Item : JsonValueList)
		{
			result.AddUnique(Item->AsString());
		}
		return result;
	};

	result.CookPlatforms = DeserializeArray(TEXT("CookPlatforms"));
	result.CookMaps = DeserializeArray(TEXT("CookMaps"));
	result.CookFilter = DeserializeArray(TEXT("CookFilter"));
	result.CookSettings = DeserializeArray(TEXT("CookSettings"));
	result.Options = JsonObject->GetStringField(TEXT("Options"));
	result.bCookAllMap = JsonObject->GetBoolField(TEXT("bCookAllMap"));
	return result;
}


bool UFlibPatchParserHelper::GetCookProcCommandParams(const FCookerConfig& InConfig, FString& OutParams)
{
	FString FinalParams;
	FinalParams.Append(FString::Printf(TEXT("\"%s\" "), *InConfig.ProjectPath));
	FinalParams.Append(FString::Printf(TEXT("%s "), *InConfig.EngineParams));

	auto CombineParamsLambda = [&FinalParams,&InConfig](const FString& InName, const TArray<FString>& InArray)
	{
		FinalParams.Append(InName);

		for (int32 Index = 0; Index < InArray.Num(); ++Index)
		{
			FinalParams.Append(InArray[Index]);
			if (!(Index == InArray.Num() - 1))
			{
				FinalParams.Append(TEXT("+"));
			}
		}
		FinalParams.Append(TEXT(" "));
	};
	CombineParamsLambda(TEXT("-targetplatform="), InConfig.CookPlatforms);
	CombineParamsLambda(TEXT("-Map="), InConfig.CookMaps);
	
	for (const auto& CookFilter : InConfig.CookFilter)
	{
		FString FilterFullPath;
		if (UFLibAssetManageHelperEx::ConvRelativeDirToAbsDir(CookFilter, FilterFullPath))
		{
			if (FPaths::DirectoryExists(FilterFullPath))
			{
				FinalParams.Append(FString::Printf(TEXT("-COOKDIR=\"%s\" "), *FilterFullPath));
			}
		}
	}

	for (const auto& Option : InConfig.CookSettings)
	{
		FinalParams.Append(TEXT("-") + Option + TEXT(" "));
	}

	FinalParams.Append(InConfig.Options);

	OutParams = FinalParams;

	return true;
}

FString UFlibPatchParserHelper::MountPathToRelativePath(const FString& InMountPath)
{
	FString Path;
	FString filename;
	FString RelativePath;
#define RELATIVE_STR_LENGTH 9
	if (InMountPath.StartsWith("../../../"))
	{
		RelativePath = UKismetStringLibrary::GetSubstring(InMountPath, RELATIVE_STR_LENGTH, InMountPath.Len() - RELATIVE_STR_LENGTH);
	}
	FString extersion;
	FPaths::Split(RelativePath, Path, filename, extersion);
	return Path/filename;
}


#include "ShaderCodeLibrary.h"

void UFlibPatchParserHelper::ReloadShaderbytecode()
{
	FShaderCodeLibrary::OpenLibrary("Global", FPaths::ProjectContentDir());
	FShaderCodeLibrary::OpenLibrary(FApp::GetProjectName(), FPaths::ProjectContentDir());
}