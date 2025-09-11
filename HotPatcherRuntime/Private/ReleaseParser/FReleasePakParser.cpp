#include "FReleasePakParser.h"
#include "HotPatcherLog.h"
#include "FlibPakHelper.h"
#include "FlibPatchParserHelper.h"
#include "Misc/EngineVersionComparison.h"

void FReleasePakParser::Parser(TSharedPtr<FReleaseParserConf> ParserConf, EHashCalculator HashCalculator)
{
	result.Platform = ParserConf->TargetPlatform;
	FReleasePakFilesConf* Conf = (FReleasePakFilesConf*)ParserConf.Get();
	
	TArray<FString> InnerPakFiles;
	for(const auto& pakFile:Conf->PakFiles)
	{
		InnerPakFiles.Append(UFlibPakHelper::GetPakFileList(pakFile,Conf->AESKey));
	}

	for(const auto& MountFile:InnerPakFiles)
	{
		FString Extersion = FPaths::GetExtension(MountFile,true);
		bool bIsWPFiles = false;
#if UE_VERSION_NEWER_THAN(5,0,0)
		bIsWPFiles = MountFile.Contains(TEXT("/_Generated_/"));
#endif
		if(!bIsWPFiles && UFlibPatchParserHelper::GetCookedUassetExtensions().Contains(Extersion))
		{
			if (!UFlibPatchParserHelper::GetUnCookUassetExtensions().Contains(Extersion))
				continue;
			// is uasset or umap
			FPatcherSpecifyAsset SpecifyAsset;
			FString AbsFile = UFlibPatchParserHelper::AssetMountPathToAbs(MountFile);
			FString LongPackageName;
			if(FPackageName::TryConvertFilenameToLongPackageName(AbsFile,LongPackageName))
			{
				FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(LongPackageName);
				FPatcherSpecifyAsset currentAsset;
				currentAsset.Asset = FSoftObjectPath(PackagePath);
				currentAsset.bAnalysisAssetDependencies = false;
				if(currentAsset.Asset.IsValid())
				{
					result.Assets.Add(currentAsset);
				}
			}
			else
			{
				UE_LOG(LogHotPatcher,Warning,TEXT("%s conv to abs path failed!"),*MountFile);
			}
		}
		else
		{
			// not uasset
			FExternFileInfo currentFile;
			FString RelativePath = MountFile;
			FString ModuleName;
			FString ModuleAbsPath;
			while(RelativePath.RemoveFromStart(TEXT("../")));
			
			UFlibAssetManageHelper::GetModuleNameByRelativePath(RelativePath,ModuleName);
			UFlibAssetManageHelper::GetEnableModuleAbsDir(ModuleName,ModuleAbsPath);
			RelativePath.RemoveFromStart(ModuleName);
			FString FinalFilePath = FPaths::Combine(ModuleAbsPath,RelativePath);
			FPaths::NormalizeFilename(FinalFilePath);
			FinalFilePath = FinalFilePath.Replace(TEXT("//"),TEXT("/"));
			if(FPaths::FileExists(FinalFilePath))
			{
				currentFile.SetFilePath(FinalFilePath);
				currentFile.MountPath = MountFile;
				currentFile.GenerateFileHash(HashCalculator);
				result.ExternFiles.AddUnique(currentFile);
			}
		}
	}
}
