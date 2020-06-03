#include "CreatePatch/ExportPatchSettings.h"
#include "FLibAssetManageHelperEx.h"

// engine header
#include "Dom/JsonValue.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/KismetStringLibrary.h"




UExportPatchSettings::UExportPatchSettings()
	: bAnalysisFilterDependencies(true),
	AssetRegistryDependencyTypes(TArray<EAssetRegistryDependencyTypeEx>{EAssetRegistryDependencyTypeEx::Packages}),
	bEnableExternFilesDiff(true), 
	UnrealPakOptions{ TEXT("-compress") ,TEXT("-compressionformats=Zlib")}
{
	PakVersionFileMountPoint = FPaths::Combine(
		TEXT("../../../"),
		UFlibPatchParserHelper::GetProjectName(),
		TEXT("Extention/Versions")
	);
}

TArray<FString> UExportPatchSettings::GetAssetIncludeFiltersPaths()const
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

TArray<FString> UExportPatchSettings::GetAssetIgnoreFiltersPaths()const
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

FString UExportPatchSettings::GetPakCommandsSaveToPath(const FString& InSaveAbsPath,const FString& InPlatfornName, const FHotPatcherVersion& InVersion)
{
	FString SavePakCommandPath = FPaths::Combine(
		InSaveAbsPath,
		InPlatfornName,
		!InVersion.BaseVersionId.IsEmpty()?
		FString::Printf(TEXT("PakList_%s_%s_%s_PakCommands.txt"), *InVersion.BaseVersionId, *InVersion.VersionId, *InPlatfornName):
		FString::Printf(TEXT("PakList_%s_%s_PakCommands.txt"), *InVersion.VersionId, *InPlatfornName)	
	);

	return SavePakCommandPath;
}


TArray<FString> UExportPatchSettings::MakeAllExternDirectoryAsPakCommand() const
{
	
	TArray<FString> CookCommandResault;
	if (!GetAddExternDirectory().Num())
		return CookCommandResault;


	TArray<FExternAssetFileInfo>&& ExFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(GetAddExternDirectory());

	for (const auto& File : ExFiles)
	{
		CookCommandResault.Add(FString::Printf(TEXT("\"%s\" \"%s\""), *File.FilePath.FilePath, *File.MountPath));
	}
	return CookCommandResault;
}

TArray<FString> UExportPatchSettings::MakeAllPakCommandsByTheSetting(const FString& InPlatformName, const FPatchVersionDiff& InVersionDiff, bool bDiffExFiles) const
{

	FAssetDependenciesInfo AllChangedAssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(InVersionDiff.AddAssetDependInfo, InVersionDiff.ModifyAssetDependInfo);

	TArray<FExternAssetFileInfo> AllChangedExternalFiles;
	AllChangedExternalFiles.Append(InVersionDiff.AddExternalFiles);
	AllChangedExternalFiles.Append(InVersionDiff.ModifyExternalFiles);

	// combine all cook commands
	{
		FString ProjectDir = UKismetSystemLibrary::GetProjectDirectory();
		
		// generated cook command form asset list
		TArray<FString> OutPakCommand;
		UFLibAssetManageHelperEx::MakePakCommandFromAssetDependencies(ProjectDir, InPlatformName, AllChangedAssetInfo, TArray<FString>{}, OutPakCommand);

		// generated cook command form project ini/AssetRegistry.bin/GlobalShaderCache*.bin
		// and all extern file
		{
			TArray<FString> AllExternPakCommand;
			TArray<FString> PakOptions;
			this->MakeAllExternAssetAsPakCommands(ProjectDir, InPlatformName, AllExternPakCommand,PakOptions);

			if (!!AllExternPakCommand.Num())
			{
				OutPakCommand.Append(AllExternPakCommand);
			}

			// external not-asset files
			{
				TArray<FExternAssetFileInfo> ExFiles;
				if (bDiffExFiles)
				{
					ExFiles = AllChangedExternalFiles;
				}
				else
				{
					ExFiles = GetAllExternFiles();
				
				}
				for (const auto& File : ExFiles)
				{
					// UE_LOG(LogTemp, Warning, TEXT("CookCommand %s"), *FString::Printf(TEXT("\"%s\" \"%s\""), *File.FilePath.FilePath, *File.MountPath));
					OutPakCommand.Add(FString::Printf(TEXT("\"%s\" \"%s\""), *File.FilePath.FilePath, *File.MountPath));
				}
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
		this->GetAssetIncludeFiltersPaths(),
		this->GetAssetIgnoreFiltersPaths(),
		this->GetAssetRegistryDependencyTypes(),
		this->GetIncludeSpecifyAssets(),
		this->GetAllExternFiles(true),
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

bool UExportPatchSettings::MakeAllExternAssetAsPakCommands(const FString& InProjectDir, const FString& InPlatform, const TArray<FString>& PakOptions, TArray<FString>& OutCookCommands)const
{
	OutCookCommands.Reset();

	FPakInternalInfo InternalAssets;
	InternalAssets.bIncludeAssetRegistry = IsIncludeAssetRegistry();
	InternalAssets.bIncludeGlobalShaderCache = IsIncludeGlobalShaderCache();
	InternalAssets.bIncludeShaderBytecode = IsIncludeShaderBytecode();

	InternalAssets.bIncludeEngineIni = IsIncludeEngineIni();
	InternalAssets.bIncludePluginIni = IsIncludePluginIni();
	InternalAssets.bIncludeProjectIni = IsIncludeProjectIni();

	OutCookCommands = UFlibPatchParserHelper::GetPakCommandsFromInternalInfo(InternalAssets, InPlatform, PakOptions);

	return true;
}

bool UExportPatchSettings::SerializePatchConfigToJsonObject(TSharedPtr<FJsonObject>& OutJsonObject)const
{
	return UFlibHotPatcherEditorHelper::SerializePatchConfigToJsonObject(this, OutJsonObject);
}

bool UExportPatchSettings::SerializePatchConfigToString(FString& OutSerializedStr)const
{
	TSharedPtr<FJsonObject> PatchConfigJsonObject = MakeShareable(new FJsonObject);
	SerializePatchConfigToJsonObject(PatchConfigJsonObject);
	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutSerializedStr);
	return FJsonSerializer::Serialize(PatchConfigJsonObject.ToSharedRef(), JsonWriter);
}



TArray<FString> UExportPatchSettings::MakeAddExternFileToPakCommands()const
{
	TArray<FString> resault;
	// FString ProjectName = UFlibPatchParserHelper::GetProjectName();
	for (const auto& ExternFile : GetAddExternFiles())
	{
		FString FileAbsPath = FPaths::ConvertRelativePathToFull(ExternFile.FilePath.FilePath);
		FPaths::MakeStandardFilename(FileAbsPath);

		if (FPaths::FileExists(FileAbsPath) &&
			ExternFile.MountPath.StartsWith(FPaths::Combine(TEXT("../../..")/*,ProjectName*/))
		)
		{
			resault.AddUnique(
				FString::Printf(TEXT("\"%s\" \"%s\""),*FileAbsPath,*ExternFile.MountPath)
			);
		}
		else
		{
			UE_LOG(LogTemp,Warning,TEXT("File mount point is invalid,is %s"), *ExternFile.MountPath)
		}
	}
	return resault;
}

TArray<FString> UExportPatchSettings::GetPakTargetPlatformNames() const
{
	TArray<FString> Resault;
	for (const auto &Platform : this->GetPakTargetPlatforms())
	{
		Resault.Add(UFlibPatchParserHelper::GetEnumNameByValue(Platform));
	}
	return Resault;
}
