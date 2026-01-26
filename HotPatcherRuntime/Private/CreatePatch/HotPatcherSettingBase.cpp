#include "CreatePatch/HotPatcherSettingBase.h"
#include "FlibPatchParserHelper.h"
#include "FPlatformExternFiles.h"
#include "HotPatcherLog.h"

#include "Misc/EngineVersionComparison.h"

FHotPatcherSettingBase::FHotPatcherSettingBase()
{
	GetAssetScanConfigRef().bAnalysisFilterDependencies = true;
	GetAssetScanConfigRef().AssetRegistryDependencyTypes = TArray<EAssetRegistryDependencyTypeEx>{EAssetRegistryDependencyTypeEx::Packages};
	GetAssetScanConfigRef().ForceSkipContentRules.Append(UFlibPatchParserHelper::GetDefaultForceSkipContentDir());
	SavePath.Path = TEXT("[PROJECTDIR]/Saved/HotPatcher/");
}


TArray<FPlatformExternAssets>& FHotPatcherSettingBase::GetAddExternAssetsToPlatform()
{
	static TArray<FPlatformExternAssets> PlatformNoAssets;
	return PlatformNoAssets;
};


void FHotPatcherSettingBase::Init()
{
	
}

TArray<FExternFileInfo> FHotPatcherSettingBase::GetAllExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash)
{
	TArray<FExternFileInfo> AllExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(GetAddExternDirectoryByPlatform(InTargetPlatform),HashCalculator,InGeneratedHash);
	
	for (auto& ExFile : GetAddExternFilesByPlatform(InTargetPlatform,InGeneratedHash))
	{
		if (!AllExternFiles.Contains(ExFile))
		{
			AllExternFiles.Add(ExFile);
		}
	}
	return AllExternFiles;
}
	
TMap<ETargetPlatform,FPlatformExternFiles> FHotPatcherSettingBase::GetAllPlatfotmExternFiles(bool InGeneratedHash)
{
	TMap<ETargetPlatform,FPlatformExternFiles> result;
	
	for(const auto& Platform:GetAddExternAssetsToPlatform())
	{
		FPlatformExternFiles PlatformIns{Platform.TargetPlatform,GetAllExternFilesByPlatform(Platform.TargetPlatform,InGeneratedHash)};
		result.Add(Platform.TargetPlatform,PlatformIns);
	}
	return result;
}
	
TArray<FExternFileInfo> FHotPatcherSettingBase::GetAddExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash)
{
	TArray<FExternFileInfo> result;
	for(const auto& Platform:GetAddExternAssetsToPlatform())
	{
		if (Platform.TargetPlatform == InTargetPlatform)
		{
			for(const auto& File:Platform.AddExternFileToPak)
			{
				uint32 index = result.Emplace(File);
				if(InGeneratedHash)
				{
					result[index].GenerateFileHash(HashCalculator);
				}
			}
		}
	}

	return result;
}
TArray<FExternDirectoryInfo> FHotPatcherSettingBase::GetAddExternDirectoryByPlatform(ETargetPlatform InTargetPlatform)
{
	TArray<FExternDirectoryInfo> result;
	for(const auto& Platform:GetAddExternAssetsToPlatform())
	{
		if (Platform.TargetPlatform == InTargetPlatform)
		{
			for(const auto& Dir:Platform.AddExternDirectoryToPak)
			{
				uint32 index = result.Emplace(Dir);
				result[index].DirectoryPath.Path = UFlibPatchParserHelper::ReplaceMarkPath(Dir.DirectoryPath.Path);
			}
		}
	}

	return result;
}

FString FHotPatcherSettingBase::GetSaveAbsPath()const
{
	if (!SavePath.Path.IsEmpty())
	{
		return UFlibPatchParserHelper::ReplaceMarkPath(GetSavePath());
	}
	return TEXT("");
}

FString FHotPatcherSettingBase::GetCombinedAdditionalCommandletArgs() const
{
	FString Result;
	TArray<FString> Options = GetAdditionalCommandletArgs();
#if UE_VERSION_OLDER_THAN(5,0,0)
	Options.AddUnique(TEXT("-NoPostLoadCacheDDC"));
#endif
	Result = UFlibPatchParserHelper::MergeOptionsAsCmdline(Options);
	return Result;
}

TArray<FString> FHotPatcherSettingBase::GetAllSkipContents() const
{
	TArray<FString> AllSkipContents;;
	AllSkipContents.Append(UFlibAssetManageHelper::DirectoriesToStrings(GetForceSkipContentRules()));
	AllSkipContents.Append(UFlibAssetManageHelper::SoftObjectPathsToStrings(GetForceSkipAssets()));
	return AllSkipContents;
}

