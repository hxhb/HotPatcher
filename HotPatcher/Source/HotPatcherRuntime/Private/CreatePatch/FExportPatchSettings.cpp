#include "CreatePatch/FExportPatchSettings.h"
#include "FLibAssetManageHelperEx.h"
#include "HotPatcherLog.h"

// engine header
#include "UObject/SavePackage.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformFilemanager.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Serialization/BulkDataManifest.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"


FExportPatchSettings::FExportPatchSettings()
	: bAnalysisFilterDependencies(true),
	AssetRegistryDependencyTypes(TArray<EAssetRegistryDependencyTypeEx>{EAssetRegistryDependencyTypeEx::Packages}),
	bEnableExternFilesDiff(true),
	DefaultPakListOptions{ TEXT("-compress") },
	DefaultCommandletOptions{ TEXT("-compress") ,TEXT("-compressionformats=Zlib")}
{
	// IoStoreSettings.IoStoreCommandletOptions.Add(TEXT("-CreateGlobalContainer="));
	PakVersionFileMountPoint = FPaths::Combine(
		TEXT("../../../"),
		UFlibPatchParserHelper::GetProjectName(),
		TEXT("Versions/version.json")
	);
	TArray<FString> DefaultSkipEditorContentRules = {TEXT("/Engine/Editor"),TEXT("/Engine/VREditor")};
	for(const auto& Ruls:DefaultSkipEditorContentRules)
	{
		FDirectoryPath PathIns;
		PathIns.Path = Ruls;
		ForceSkipContentRules.Add(PathIns);
	}
}

void FExportPatchSettings::Init()
{
	Super::Init();
	auto AppendOptionsLambda = [](TArray<FString>& SrcOptions,const TArray<FString>& AppendOptions)
	{
		for(const auto& Option:AppendOptions)
		{
			SrcOptions.AddUnique(Option);
		}
	};
	AppendOptionsLambda(UnrealPakSettings.UnrealPakListOptions,GetDefaultPakListOptions());
	AppendOptionsLambda(UnrealPakSettings.UnrealCommandletOptions,GetDefaultCommandletOptions());
	AppendOptionsLambda(IoStoreSettings.IoStorePakListOptions,GetDefaultPakListOptions());
	AppendOptionsLambda(IoStoreSettings.IoStoreCommandletOptions,GetDefaultCommandletOptions());
	InitPlatformPackageContexts();
}

void FExportPatchSettings::InitPlatformPackageContexts()
{
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	TArray<ITargetPlatform*> CookPlatforms;
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	for (ITargetPlatform *TargetPlatform : TargetPlatforms)
	{
		if (GetPakTargetPlatformNames().Contains(TargetPlatform->PlatformName()))
		{
			CookPlatforms.AddUnique(TargetPlatform);
			const FString PlatformString = TargetPlatform->PlatformName();

			// const FString ResolvedRootPath = RootPathSandbox.Replace(TEXT("[Platform]"), *PlatformString);
			const FString ResolvedProjectPath = FPaths::Combine(ProjectPath,FString::Printf(TEXT("Saved/Cooked/%s/%s"),*TargetPlatform->PlatformName(),FApp::GetProjectName()));

			FPackageStoreBulkDataManifest* BulkDataManifest	= new FPackageStoreBulkDataManifest(ResolvedProjectPath);
			FLooseFileWriter* LooseFileWriter				= GetIoStoreSettings().bIoStore ? new FLooseFileWriter() : nullptr;

			FConfigFile PlatformEngineIni;
			FConfigCacheIni::LoadLocalIniFile(PlatformEngineIni, TEXT("Engine"), true, *TargetPlatform->IniPlatformName());
		
#if ENGINE_MAJOR_VERSION >4 || ENGINE_MINOR_VERSION > 25
			bool bLegacyBulkDataOffsets = false;
			PlatformEngineIni.GetBool(TEXT("Core.System"), TEXT("LegacyBulkDataOffsets"), bLegacyBulkDataOffsets);
			FSavePackageContext* SavePackageContext	= new FSavePackageContext(LooseFileWriter, BulkDataManifest, bLegacyBulkDataOffsets);
#else
			FSavePackageContext* SavePackageContext	= new FSavePackageContext(LooseFileWriter, BulkDataManifest);
#endif
			ETargetPlatform Platform;
			UFlibPatchParserHelper::GetEnumValueByName(TargetPlatform->PlatformName(),Platform);
			PlatformSavePackageContexts.Add(Platform,SavePackageContext);
		}
	}
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

TArray<FExternFileInfo> FExportPatchSettings::GetAllExternFiles(bool InGeneratedHash)const
{
	TArray<FExternFileInfo> AllExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(GetAddExternDirectory());

	for (auto& ExFile : GetAddExternFiles())
	{
		if (!AllExternFiles.Contains(ExFile))
		{
			AllExternFiles.Add(ExFile);
		}
	}
	if (InGeneratedHash)
	{
		for (auto& ExFile : AllExternFiles)
		{
			ExFile.GenerateFileHash();
		}
	}
	return AllExternFiles;
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
	this->GetBaseVersionInfo(BaseVersionInfo);

	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfo(
        this->GetVersionId(),
        BaseVersionInfo.VersionId,
        FDateTime::UtcNow().ToString(),
        this->GetAssetIncludeFiltersPaths(),
        this->GetAssetIgnoreFiltersPaths(),
        this->GetAssetRegistryDependencyTypes(),
        this->GetIncludeSpecifyAssets(),
        this->GetAddExternAssetsToPlatform(),
        // this->GetAllExternFiles(true),
        this->GetAssetsDependenciesScanedCaches(),
        this->IsIncludeHasRefAssetsOnly()
    );

	return CurrentVersion;
}

bool FExportPatchSettings::GetBaseVersionInfo(FHotPatcherVersion& OutBaseVersion) const
{
	FString BaseVersionContent;

	bool bDeserializeStatus = false;
	if (this->IsByBaseVersion())
	{
		if (UFLibAssetManageHelperEx::LoadFileToString(this->GetBaseVersion(), BaseVersionContent))
		{
			bDeserializeStatus = UFlibPatchParserHelper::TDeserializeJsonStringAsStruct(BaseVersionContent, OutBaseVersion);
		}
	}

	return bDeserializeStatus;
}


FString FExportPatchSettings::GetCurrentVersionSavePath() const
{
	FString CurrentVersionSavePath = FPaths::Combine(this->GetSaveAbsPath(), const_cast<FExportPatchSettings*>(this)->GetNewPatchVersionInfo().VersionId);
	return CurrentVersionSavePath;
}

TArray<FString> FExportPatchSettings::GetPakTargetPlatformNames() const
{
	TArray<FString> Resault;
	for (const auto &Platform : this->GetPakTargetPlatforms())
	{
		Resault.Add(UFlibPatchParserHelper::GetEnumNameByValue(Platform));
	}
	return Resault;
}

TArray<FString> FExportPatchSettings::GetAssetIgnoreFiltersPaths()const
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

TArray<FString> FExportPatchSettings::GetAssetIncludeFiltersPaths()const
{
	TArray<FString> Result;
	for (const auto& Filter : AssetIncludeFilters)
	{
		if (!Filter.Path.IsEmpty())
		{
			Result.AddUnique(Filter.Path);
		}
	}
	return Result;
}

bool FExportPatchSettings::SavePlatformBulkDataManifest(ETargetPlatform Platform)
{
	bool bRet = false;
	if(!GetPlatformSavePackageContexts().Contains(Platform))
		return bRet;
	FSavePackageContext* PackageContext =* GetPlatformSavePackageContexts().Find(Platform);
	if (PackageContext != nullptr && PackageContext->BulkDataManifest != nullptr)
	{
		PackageContext->BulkDataManifest->Save();
		bRet = true;
	}
	return bRet;
}

TArray<FString> FExportPatchSettings::GetForceSkipContentStrRules()const
{
	TArray<FString> Path;
	for(const auto& DirPath:GetForceSkipContentRules())
	{
		Path.AddUnique(DirPath.Path);
	}
	return Path;
}

TArray<FString> FExportPatchSettings::GetForceSkipAssetsStr()const
{
	TArray<FString> result;
	for(const auto &Asset:GetForceSkipAssets())
	{
		result.Add(Asset.GetLongPackageName());
	}
	return result;
}

