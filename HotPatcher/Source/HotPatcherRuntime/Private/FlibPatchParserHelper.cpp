// Fill out your copyright notice in the Description page of Project Settings.

// project header
#include "FlibPatchParserHelper.h"
#include "FlibAssetManageHelperEx.h"
#include "Struct/AssetManager/FFileArrayDirectoryVisitor.hpp"

// engine header
#include "Misc/SecureHash.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "Engine/EngineTypes.h"
#include "Misc/Paths.h"

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
			UE_LOG(LogTemp, Warning, TEXT("External Files num %d"), AllExternalFilesJsonObj.Num());

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

	auto ParserAddFiles = [](const FHotPatcherVersion& InNewVersion, const FHotPatcherVersion& InBaseVersion, TArray<FExternAssetFileInfo>& OutAddFiles)
	{
		TArray<FString> NewVersionFilesKeys;
		InNewVersion.ExternalFiles.GetKeys(NewVersionFilesKeys);

		TArray<FString> BaseVersionFilesKeys;
		InBaseVersion.ExternalFiles.GetKeys(BaseVersionFilesKeys);

		for (const auto& NewVersionFile : NewVersionFilesKeys)
		{
			if (!InBaseVersion.ExternalFiles.Contains(NewVersionFile))
			{
				OutAddFiles.Add(*InNewVersion.ExternalFiles.Find(NewVersionFile));
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
			UE_LOG(LogTemp, Log, TEXT("check file %s."), *NewVersionFile);
			if (InBaseVersion.ExternalFiles.Contains(NewVersionFile))
			{
				const FExternAssetFileInfo& NewFile = *InBaseVersion.ExternalFiles.Find(NewVersionFile);
				const FExternAssetFileInfo& BaseFile = *InNewVersion.ExternalFiles.Find(NewVersionFile);
				bool bIsSame = NewFile == BaseFile;
				if (!bIsSame)
				{
					UE_LOG(LogTemp, Log, TEXT("%s is same."), *NewFile.MountPath);
					OutModifyFiles.Add(*InNewVersion.ExternalFiles.Find(NewVersionFile));
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("%s is not same."), *NewFile.MountPath);
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

FString UFlibPatchParserHelper::SerializeDiffAssetsInfomationToString(const FAssetDependenciesInfo& InAddAsset,
	const FAssetDependenciesInfo& InModifyAsset,
	const FAssetDependenciesInfo& InDeleteAsset)
{
	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);

	FString SerializeDiffInfo;
	{
		// is empty Info
		auto IsEmptyInfo = [](const FAssetDependenciesInfo& InAssetInfo)->bool
		{
			TArray<FString> Keys;
			InAssetInfo.mDependencies.GetKeys(Keys);

			return Keys.Num() == 0;
		};

		auto SerializeAssetInfoToJson = [&RootJsonObject,&IsEmptyInfo](const FAssetDependenciesInfo& InAssetInfo, const FString& InDescrible)->bool
		{
			bool bRunStatus = false;
			if (!IsEmptyInfo(InAssetInfo))
			{
				TSharedPtr<FJsonObject> AssetsJsonObject = MakeShareable(new FJsonObject);
				bRunStatus = UFLibAssetManageHelperEx::SerializeAssetDependenciesToJsonObject(InAssetInfo, AssetsJsonObject, TArray<FString>{"Script"});
				if (bRunStatus)
				{
					RootJsonObject->SetObjectField(InDescrible, AssetsJsonObject);
				}
			}
			return bRunStatus;
		};

		SerializeAssetInfoToJson(InAddAsset, TEXT("AddedAssets"));
		SerializeAssetInfoToJson(InModifyAsset, TEXT("ModifiedAssets"));
		SerializeAssetInfoToJson(InDeleteAsset, TEXT("DeletedAssets"));
	}

	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&SerializeDiffInfo);
	FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter);

	return SerializeDiffInfo;
}


FString UFlibPatchParserHelper::SerializeDiffExternalFilesInfomationToString(
	const TArray<FExternAssetFileInfo>& InAddFiles,
	const TArray<FExternAssetFileInfo>& InModifyFiles,
	const TArray<FExternAssetFileInfo>& InDeleteFiles)
{
	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);
	auto ParserFilesWithDescribleLambda = [&RootJsonObject](const TArray<FExternAssetFileInfo>& InFiles,const FString& InDescrible)
	{
		TArray<TSharedPtr<FJsonValue>> FileJsonValueList;
		for (const auto& File : InFiles)
		{
			FileJsonValueList.Add(MakeShareable(new FJsonValueString(File.MountPath)));
		}
		RootJsonObject->SetArrayField(InDescrible,FileJsonValueList);
	};
	

	ParserFilesWithDescribleLambda(InAddFiles, TEXT("AddFiles"));
	ParserFilesWithDescribleLambda(InModifyFiles, TEXT("ModifyFiles"));
	ParserFilesWithDescribleLambda(InDeleteFiles, TEXT("DeleteFiles"));

	FString SerializeDiffInfo;
	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&SerializeDiffInfo);
	FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), JsonWriter);

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

TArray<FString> UFlibPatchParserHelper::GetProjectIniFiles(const FString& InProjectDir)
{
	TArray<FString> Resault;
	FString ConfigFolder = FPaths::Combine(InProjectDir, TEXT("Config"));
	if (FPaths::DirectoryExists(InProjectDir) && FPaths::DirectoryExists(ConfigFolder))
	{
		TArray<FString> FoundAllIniFiles;
		IFileManager::Get().FindFiles(FoundAllIniFiles, *ConfigFolder, TEXT("ini"));
		
		for (const auto& IniFile:FoundAllIniFiles)
		{
			if(!IniFile.StartsWith(TEXT("DefaultEditor")))
				Resault.AddUnique(FPaths::Combine(ConfigFolder, IniFile));
		}
	}
	return Resault;
}

bool UFlibPatchParserHelper::ConvIniFilesToCookCommands(const FString& InEngineAbsDir,const FString& InProjectAbsDir,const FString& InProjectName, const TArray<FString>& InIniFiles, TArray<FString>& OutCommands)
{
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
				TEXT("\"%s\" \"%s\""),
					*IniFile,
					*CookedIniRelativePath
			);
			OutCommands.AddUnique(CookCommand);
	
			bRunStatus = true;
		}
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::ConvNotAssetFileToCookCommand(const FString& InProjectDir, const FString& InPlatformName, const FString& InCookedFile, FString& OutCommand)
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

		OutCommand = FString::Printf(
			TEXT("\"%s\" \"%s\""),
			*InCookedFile,
			*AssetFileRelativeCookPath
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

TArray<FString> UFlibPatchParserHelper::GetEngineConfigs(const FString& InPlatformName)
{
	TArray<FString> Result;
	const FString EngineConfigAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineConfigDir());

	if (FPaths::DirectoryExists(EngineConfigAbsDir))
	{
		FFillArrayDirectoryVisitor Visitor;
		
		IFileManager::Get().IterateDirectory(*EngineConfigAbsDir, Visitor);

		for (const auto& IniFile : Visitor.Files)
		{
			if (!FPaths::GetCleanFilename(IniFile).Contains(TEXT("Editor")))
			{
				Result.Add(IniFile);
			}
		}
		int32 PlatformNameBeginIndex= EngineConfigAbsDir.Len() + 1;
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
			
			if (FPaths::DirectoryExists(PluginIniPath))
			{
				FFillArrayDirectoryVisitor PluginIniVisitor;
				IFileManager::Get().IterateDirectory(*PluginIniPath, PluginIniVisitor);

				for (const auto& IniFile : PluginIniVisitor.Files)
				{
					result.AddUnique(IniFile);
				}

				for (const auto& IniDir : PluginIniVisitor.Directories)
				{
					FString ChildFolderName  = UKismetStringLibrary::GetSubstring(IniDir, PluginIniPath.Len(), IniDir.Len() - PluginIniPath.Len());
					
					if (InPlatformName.Contains(ChildFolderName))
					{
						FFillArrayDirectoryVisitor PluginChildIniVisitor;
						IFileManager::Get().IterateDirectoryRecursively(*IniDir, PluginChildIniVisitor);
						
						for (const auto& IniFile : PluginChildIniVisitor.Files)
						{
							result.AddUnique(IniFile);
						}
					}
				}
			}
			
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
	OutJsonObject->SetStringField(TEXT("Asset"), InSpecifyAsset.Asset.GetLongPackageName());
	OutJsonObject->SetBoolField(TEXT("bAnalysisAssetDependencies"), InSpecifyAsset.bAnalysisAssetDependencies);
	bRunStatus = true;
	return bRunStatus;
}

