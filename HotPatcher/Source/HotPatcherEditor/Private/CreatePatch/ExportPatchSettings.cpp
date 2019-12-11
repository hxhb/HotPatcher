#include "ExportPatchSettings.h"
#include "FLibAssetManageHelperEx.h"
#include "HAL/PlatformFilemanager.h"

UExportPatchSettings::UExportPatchSettings()
{
	
	PakVersionFileMountPoint = FPaths::Combine(
		TEXT("../../../"),
		UFlibPatchParserHelper::GetProjectName(),
		TEXT("Extention/Versions")
	);
}

TArray<FString> UExportPatchSettings::GetAssetIncludeFilters()const
{
	TArray<FString> Result;
	for (const auto& Filter : AssetIncludeFilters)
	{
		if(!Filter.Path.IsEmpty())
		{
			Result.AddUnique(Filter.Path);
		}
	}
	return Result;
}

TArray<FString> UExportPatchSettings::GetAssetIgnoreFilters()const
{
	TArray<FString> Result;
	for (const auto& Filter : AssetIgnoreFilters)
	{
		if (!Filter.Path.IsEmpty())
		{
			Result.AddUnique(Filter.Path);
		}
	}
	return Result;
}

FString UExportPatchSettings::GetSaveAbsPath()const
{
	if (!SavePath.Path.IsEmpty())
	{
		return FPaths::ConvertRelativePathToFull(SavePath.Path);
	}
	return TEXT("");
}


FPakVersion UExportPatchSettings::GetPakVersion(const FHotPatcherVersion& InHotPatcherVersion, const FString& InUtcTime)
{
	FPakVersion PakVersion;
	PakVersion.BaseVersionId = InHotPatcherVersion.BaseVersionId;
	PakVersion.VersionId = InHotPatcherVersion.VersionId;
	PakVersion.Date = InUtcTime;

	// encode BaseVersionId_VersionId_Data to SHA1
	PakVersion.CheckCode = UFlibPatchParserHelper::HashStringWithSHA1(
		FString::Printf(
			TEXT("%s_%s_%s"),
			*PakVersion.BaseVersionId,
			*PakVersion.VersionId,
			*PakVersion.Date
		)
	);

	return PakVersion;
}

FString UExportPatchSettings::GetSavePakVersionPath(const FString& InSaveAbsPath, const FHotPatcherVersion& InVersion)
{
	FString PatchSavePath(InSaveAbsPath);
	FPaths::MakeStandardFilename(PatchSavePath);

	FString SavePakVersionFilePath = FPaths::Combine(
		PatchSavePath,
		FString::Printf(
			TEXT("%s_PakVersion.json"),
			*InVersion.VersionId
		)
	);
	return SavePakVersionFilePath;
}

FString UExportPatchSettings::GetSavePakCommandsPath(const FString& InSaveAbsPath,const FString& InPlatfornName, const FHotPatcherVersion& InVersion)
{
	FString SavePakCommandPath = FPaths::Combine(
		InSaveAbsPath,
		InPlatfornName,
		FString::Printf(TEXT("PakList_%s_%s_%s_PakCommands.txt"), *InVersion.BaseVersionId, *InVersion.VersionId, *InPlatfornName)
	);

	return SavePakCommandPath;
}

TArray<FString> UExportPatchSettings::CombineAllCookCommandsInTheSetting(const FString& InPlatformName,const FAssetDependenciesInfo& AllChangedAssetInfo) const
{
	// combine all cook commands
	{
		FString ProjectDir = UKismetSystemLibrary::GetProjectDirectory();
		
		// generated cook command form asset list
		TArray<FString> OutPakCommand;
		UFLibAssetManageHelperEx::GetCookCommandFromAssetDependencies(ProjectDir, InPlatformName, AllChangedAssetInfo, TArray<FString>{}, OutPakCommand);

		// generated cook command form project ini/AssetRegistry.bin/GlobalShaderCache*.bin
		// and all extern file
		{
			TArray<FString> AllExternCookCommand;

			this->GetAllExternAssetCookCommands(ProjectDir, InPlatformName, AllExternCookCommand);

			if (!!AllExternCookCommand.Num())
			{
				OutPakCommand.Append(AllExternCookCommand);
			}


			TArray<FString> AllExternFileToPakCommands = this->CombineAddExternFileToCookCommands();

			if (!!AllExternFileToPakCommands.Num())
			{
				OutPakCommand.Append(AllExternFileToPakCommands);
			}
		}

		// add PakVersion.json to cook commands
		{
			FHotPatcherVersion CurrentNewPatchVersion = GetNewPatchVersionInfo();
			FString CurrentVersionSavePath = FPaths::Combine(this->GetSaveAbsPath(), CurrentNewPatchVersion.VersionId);

			FString SavedPakVersionFilePath = UExportPatchSettings::GetSavePakVersionPath(CurrentVersionSavePath, CurrentNewPatchVersion);

			if (this->IsIncludePakVersion() && FPaths::FileExists(SavedPakVersionFilePath))
			{
				FString FileNameWithExtension = FPaths::GetCleanFilename(SavedPakVersionFilePath);

				FString CommbinedCookCommand = FString::Printf(
					TEXT("\"%s\" \"%s\""),
					*SavedPakVersionFilePath,
					*FPaths::Combine(this->GetPakVersionFileMountPoint(), FileNameWithExtension)
				);

				OutPakCommand.Add(CommbinedCookCommand);
			}
		}

		return OutPakCommand;
	}
}

FHotPatcherVersion UExportPatchSettings::GetNewPatchVersionInfo() const
{
	FHotPatcherVersion BaseVersionInfo;
	this->GetBaseVersionInfo(BaseVersionInfo);

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfo(
		this->GetVersionId(),
		BaseVersionInfo.VersionId,
		FDateTime::UtcNow().ToString(),
		this->GetAssetIncludeFilters(),
		this->GetAssetIgnoreFilters(),
		this->IsIncludeHasRefAssetsOnly()
	);

	return CurrentVersion;
}

bool UExportPatchSettings::GetBaseVersionInfo(FHotPatcherVersion& OutBaseVersion) const
{
	FString BaseVersionContent;

	bool bDeserializeStatus = false;
	if (this->IsByBaseVersion())
	{
		if (UFLibAssetManageHelperEx::LoadFileToString(this->GetBaseVersion(), BaseVersionContent))
		{
			bDeserializeStatus = UFlibPatchParserHelper::DeserializeHotPatcherVersionFromString(BaseVersionContent, OutBaseVersion);
		}
	}

	return bDeserializeStatus;
}


FString UExportPatchSettings::GetCurrentVersionSavePath() const
{
	FString CurrentVersionSavePath = FPaths::Combine(this->GetSaveAbsPath(), GetNewPatchVersionInfo().VersionId);
	return CurrentVersionSavePath;
}

bool UExportPatchSettings::GetAllExternAssetCookCommands(const FString& InProjectDir, const FString& InPlatform, TArray<FString>& OutCookCommands)const
{
	OutCookCommands.Reset();
	TArray<FString> SearchAssetList;
	if (IsIncludeAssetRegistry())
	{
		FString AssetRegistryCookCommand;
		if (UFlibPatchParserHelper::GetCookedAssetRegistryFile(InProjectDir, UFlibPatchParserHelper::GetProjectName(), InPlatform, AssetRegistryCookCommand))
		{
		SearchAssetList.AddUnique(AssetRegistryCookCommand);
		}
	}

	if (IsIncludeGlobalShaderCache())
	{
		TArray<FString> GlobalShaderCacheList = UFlibPatchParserHelper::SearchCookedGlobalShaderCacheFiles(InProjectDir, InPlatform);
		if (!!GlobalShaderCacheList.Num())
		{
		SearchAssetList.Append(GlobalShaderCacheList);
		}
	}

	// combine as cook commands
	{
		for (const auto& AssetFile:SearchAssetList)
		{
		FString CurrentCommand;
		if (UFlibPatchParserHelper::ConvNotAssetFileToCookCommand(InProjectDir, InPlatform, AssetFile, CurrentCommand))
		{
			OutCookCommands.AddUnique(CurrentCommand);
		}
		}
	}

	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());

	auto CombineIniCookCommands = [&OutCookCommands,&InProjectDir,&EngineAbsDir,&InPlatform](const TArray<FString>& IniFiles)
	{
		TArray<FString> result;

		TArray<FString> IniCookCommmands;
		UFlibPatchParserHelper::ConvIniFilesToCookCommands(
			EngineAbsDir,
			InProjectDir,
			UFlibPatchParserHelper::GetProjectName(),
			IniFiles,
			IniCookCommmands
		);
		if (!!IniCookCommmands.Num())
		{
			OutCookCommands.Append(IniCookCommmands);
		}
		
	};

	if (IsIncludeProjectIni())
	{
		TArray<FString> ProjectIniFiles = UFlibPatchParserHelper::GetProjectIniFiles(InProjectDir);
		CombineIniCookCommands(ProjectIniFiles);

	}
	if (IsIncludeEngineIni())
	{
		TArray<FString> EngineIniFiles = UFlibPatchParserHelper::GetEngineConfigs(InPlatform);
		CombineIniCookCommands(EngineIniFiles);
	}
	if (IsIncludePluginIni())
	{
		TArray<FString> PluginIniFiles = UFlibPatchParserHelper::GetEnabledPluginConfigs(InPlatform);
		CombineIniCookCommands(PluginIniFiles);
	}

	return true;
	}

bool UExportPatchSettings::SerializePatchConfigToJsonObject(TSharedPtr<FJsonObject>& OutJsonObject)const
{
	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}
	OutJsonObject->SetStringField(TEXT("VersionId"), GetVersionId());
	OutJsonObject->SetBoolField(TEXT("bByBaseVersion"), IsByBaseVersion());
	OutJsonObject->SetStringField(TEXT("BaseVersion"), GetBaseVersion());
	
	auto SerializeArrayLambda = [&OutJsonObject](const TArray<FString>& InArray,const FString& InJsonArrayName)
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

	SerializeArrayLambda(ConvDirPathsToStrings(AssetIncludeFilters), TEXT("AssetIncludeFilter"));
	SerializeArrayLambda(ConvDirPathsToStrings(AssetIgnoreFilters), TEXT("AssetIgnoreFilter"));

	OutJsonObject->SetBoolField(TEXT("bIncludeAssetRegistry"), IsIncludeAssetRegistry());
	OutJsonObject->SetBoolField(TEXT("bIncludeGlobalShaderCache"), IsIncludeGlobalShaderCache());
	OutJsonObject->SetBoolField(TEXT("bIncludeEngineIni"), IsIncludeEngineIni());
	OutJsonObject->SetBoolField(TEXT("bIncludePluginIni"), IsIncludePluginIni());
	OutJsonObject->SetBoolField(TEXT("bIncludeProjectIni"), IsIncludeProjectIni());

	// serialize all add extern file to pak
	{
		TSharedPtr<FJsonObject> AddExFilesJsonObject = MakeShareable(new FJsonObject);
		for (const auto& ExFileInfo : GetAddExternFiles())
		{
		TSharedPtr<FJsonObject> CurrentFileJsonObject;
		UFlibHotPatcherEditorHelper::SerializeExAssetFileInfoToJsonObject(ExFileInfo, CurrentFileJsonObject);
		AddExFilesJsonObject->SetObjectField(ExFileInfo.MountPath,CurrentFileJsonObject);
		}
		OutJsonObject->SetObjectField(TEXT("AddExFilesToPak"), AddExFilesJsonObject);
	}
	
	// serialize pakversion
	{
		OutJsonObject->SetBoolField(TEXT("bIncludePakVersion"), IsIncludePakVersion());
		OutJsonObject->SetStringField(TEXT("PakVersionFileMountPoint"), GetPakVersionFileMountPoint());
	}
	// serialize UnrealPakOptions
	SerializeArrayLambda(GetUnrealPakOptions(), TEXT("UnrealPakOptions"));

	// serialize platform list
	{
		TArray<FString> AllPlatforms;
		for (const auto &Platform : GetPakTargetPlatforms())
		{
		// save UnrealPak.exe command file
		FString PlatformName;
		{	
			FString EnumName;
			StaticEnum<ETargetPlatform>()->GetNameByValue((int64)Platform).ToString().Split(TEXT("::"), &EnumName, &PlatformName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		}
		AllPlatforms.AddUnique(PlatformName);
		}
		SerializeArrayLambda(AllPlatforms, TEXT("TargetPlatforms"));
	}

	OutJsonObject->SetBoolField(TEXT("bSavePakList"), IsSavePakList());
	OutJsonObject->SetBoolField(TEXT("bSaveDiffAnalysis"), IsSaveDiffAnalysis());
	OutJsonObject->SetBoolField(TEXT("bSavePatchConfig"), IsSavePatchConfig());
	OutJsonObject->SetStringField(TEXT("SavePath"),GetSaveAbsPath());

	return true;
	}

bool UExportPatchSettings::SerializePatchConfigToString(FString& OutSerializedStr)const
{
	TSharedPtr<FJsonObject> PatchConfigJsonObject = MakeShareable(new FJsonObject);
	SerializePatchConfigToJsonObject(PatchConfigJsonObject);

	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutSerializedStr);
	return FJsonSerializer::Serialize(PatchConfigJsonObject.ToSharedRef(), JsonWriter);
}


TArray<FString> UExportPatchSettings::CombineAddExternFileToCookCommands()const
{
	TArray<FString> resault;
	FString ProjectName = UFlibPatchParserHelper::GetProjectName();
	for (const auto& ExternFile : GetAddExternFiles())
	{
		FString FileAbsPath = FPaths::ConvertRelativePathToFull(ExternFile.FilePath.FilePath);
		if (FPaths::FileExists(FileAbsPath) &&
		ExternFile.MountPath.StartsWith(FPaths::Combine(TEXT("../../.."),ProjectName))
		)
		{
		resault.AddUnique(
			FString::Printf(TEXT("\"%s\" \"%s\""),*FileAbsPath,*ExternFile.MountPath)
		);
		}
	}
	return resault;
}

TArray<FString> UExportPatchSettings::GetPakTargetPlatformNames() const
{
	TArray<FString> Resault;
	for (const auto &Platform : this->GetPakTargetPlatforms())
	{
		// save UnrealPak.exe command file
		FString PlatformName;
		{
			FString EnumName;
			StaticEnum<ETargetPlatform>()->GetNameByValue((int64)Platform).ToString().Split(TEXT("::"), &EnumName, &PlatformName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		}
		Resault.Add(PlatformName);
	}
	return Resault;
}
