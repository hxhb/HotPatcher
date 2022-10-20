#include "CreatePatch/FExportPatchSettings.h"
#include "FlibAssetManageHelper.h"
#include "HotPatcherLog.h"
#include "FlibPatchParserHelper.h"


FExportPatchSettings::FExportPatchSettings()
	:bEnableExternFilesDiff(true),
	DefaultPakListOptions{ TEXT("-compress") },
	DefaultCommandletOptions{ TEXT("-compress") ,TEXT("-compressionformats=Zlib")}
{
	// IoStoreSettings.IoStoreCommandletOptions.Add(TEXT("-CreateGlobalContainer="));
	PakVersionFileMountPoint = FPaths::Combine(
		TEXT("../../../"),
		UFlibPatchParserHelper::GetProjectName(),
		TEXT("Versions/version.json")
	);
}

void FExportPatchSettings::Init()
{
	Super::Init();
}

FString FExportPatchSettings::GetBaseVersion() const
{
	return UFlibPatchParserHelper::ReplaceMarkPath(BaseVersion.FilePath); 
}

FPakVersion FExportPatchSettings::GetPakVersion(const FHotPatcherVersion& InHotPatcherVersion, const FString& InUtcTime)
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

FString FExportPatchSettings::GetSavePakVersionPath(const FString& InSaveAbsPath, const FHotPatcherVersion& InVersion)
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

FString FExportPatchSettings::GetPakCommandsSaveToPath(const FString& InSaveAbsPath,const FString& InPlatfornName, const FHotPatcherVersion& InVersion)
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

FHotPatcherVersion FExportPatchSettings::GetNewPatchVersionInfo()
{
	FHotPatcherVersion BaseVersionInfo;
	GetBaseVersionInfo(BaseVersionInfo);

	FHotPatcherVersion CurrentVersion;
	CurrentVersion.VersionId =  GetVersionId();
	CurrentVersion.BaseVersionId = BaseVersionInfo.VersionId;
	CurrentVersion.Date = FDateTime::UtcNow().ToString();
	UFlibPatchParserHelper::RunAssetScanner(GetAssetScanConfig(),CurrentVersion);
	UFlibPatchParserHelper::ExportExternAssetsToPlatform(GetAddExternAssetsToPlatform(),CurrentVersion,false,GetHashCalculator());
	
	// UFlibPatchParserHelper::ExportReleaseVersionInfo(
 //        GetVersionId(),
 //        BaseVersionInfo.VersionId,
 //        FDateTime::UtcNow().ToString(),
 //        UFlibAssetManageHelper::DirectoryPathsToStrings(GetAssetIncludeFilters()),
	// 		 UFlibAssetManageHelper::DirectoryPathsToStrings(GetAssetIgnoreFilters()),
 //        GetAllSkipContents(),
 //        GetForceSkipClasses(),
 //        GetAssetRegistryDependencyTypes(),
 //        GetIncludeSpecifyAssets(),
 //        GetAddExternAssetsToPlatform(),
 //        IsIncludeHasRefAssetsOnly()
 //    );

	return CurrentVersion;
}

bool FExportPatchSettings::GetBaseVersionInfo(FHotPatcherVersion& OutBaseVersion) const
{
	FString BaseVersionContent;

	bool bDeserializeStatus = false;
	if (IsByBaseVersion())
	{
		if (UFlibAssetManageHelper::LoadFileToString(GetBaseVersion(), BaseVersionContent))
		{
			bDeserializeStatus = THotPatcherTemplateHelper::TDeserializeJsonStringAsStruct(BaseVersionContent, OutBaseVersion);
		}
	}

	return bDeserializeStatus;
}


FString FExportPatchSettings::GetCurrentVersionSavePath() const
{
	FString CurrentVersionSavePath = FPaths::Combine(GetSaveAbsPath(), /*const_cast<FExportPatchSettings*>(this)->GetNewPatchVersionInfo().*/VersionId);
	return CurrentVersionSavePath;
}

TArray<FString> FExportPatchSettings::GetPakTargetPlatformNames() const
{
	TArray<FString> Resault;
	for (const auto &Platform : GetPakTargetPlatforms())
	{
		Resault.Add(THotPatcherTemplateHelper::GetEnumNameByValue(Platform));
	}
	return Resault;
}

