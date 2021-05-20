#include "CreatePatch/HotPatcherSettingBase.h"
#include "FlibPatchParserHelper.h"
#include "FPlatformExternFiles.h"
#include "HotPatcherLog.h"

TArray<FDirectoryPath>& FHotPatcherSettingBase::GetAssetIncludeFilters()
{
	static TArray<FDirectoryPath> TempDir;
	return TempDir;
};
TArray<FDirectoryPath>& FHotPatcherSettingBase::GetAssetIgnoreFilters()
{
	static TArray<FDirectoryPath> TempDir;
	return TempDir;
}
TArray<FPatcherSpecifyAsset>& FHotPatcherSettingBase::GetIncludeSpecifyAssets()
{
	static TArray<FPatcherSpecifyAsset> TempAssets;
	return TempAssets;
};
TArray<FPlatformExternAssets>& FHotPatcherSettingBase::GetAddExternAssetsToPlatform()
{
	static TArray<FPlatformExternAssets> PlatformNoAssets;
	return PlatformNoAssets;
};
TMap<FString,FAssetDependenciesInfo>& FHotPatcherSettingBase::GetAssetsDependenciesScanedCaches()
{
	return ScanedCaches;
}

void FHotPatcherSettingBase::Init()
{
	GetAssetsDependenciesScanedCaches().Empty();
    GScanCacheOptimize = IsScanCacheOptimize();
    UE_LOG(LogHotPatcher, Display, TEXT("Enable Scan Cache Optimize %s"),GScanCacheOptimize?TEXT("true"):TEXT("false"));
    	
}

TArray<FExternFileInfo> FHotPatcherSettingBase::GetAllExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash)
{
	TArray<FExternFileInfo> AllExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(GetAddExternDirectoryByPlatform(InTargetPlatform));
	
	for (auto& ExFile : GetAddExternFilesByPlatform(InTargetPlatform))
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
	
TArray<FExternFileInfo> FHotPatcherSettingBase::GetAddExternFilesByPlatform(ETargetPlatform InTargetPlatform)
{
	TArray<FExternFileInfo> result;
	for(const auto& Platform:GetAddExternAssetsToPlatform())
	{
		if (Platform.TargetPlatform == InTargetPlatform)
		{
			for(const auto& File:Platform.AddExternFileToPak)
			{
				uint32 index = result.Emplace(File);
				result[index].FilePath.FilePath = UFlibPatchParserHelper::ReplaceMarkPath(File.FilePath.FilePath);
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
		return UFlibPatchParserHelper::ReplaceMarkPath(SavePath.Path);
	}
	return TEXT("");
}