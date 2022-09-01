#include "FReleasePakParser.h"
#include "HotPatcherLog.h"
#include "FlibPakHelper.h"
#include "FlibPatchParserHelper.h"

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
		if(UFlibPatchParserHelper::GetCookedUassetExtensions().Contains(Extersion))
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
			currentFile.FilePath.FilePath = FinalFilePath;
			currentFile.MountPath = MountFile;
			currentFile.GenerateFileHash(HashCalculator);
			result.ExternFiles.AddUnique(currentFile);
		}
	}
}
