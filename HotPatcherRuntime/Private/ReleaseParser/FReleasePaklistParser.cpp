#include "FReleasePaklistParser.h"
#include "Misc/FileHelper.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
#include "Kismet/KismetStringLibrary.h"
#include "Misc/EngineVersionComparison.h"


FString NormalizePaklistPath(const FString& InStr)
{
	FString resultStr = InStr;
	if(resultStr.StartsWith(TEXT("\"")))
	{
		resultStr.RemoveAt(0);
	}
	if(resultStr.EndsWith(TEXT("\"")))
	{
		resultStr.RemoveAt(resultStr.Len() - 1);
	}
	return resultStr;
};

struct FPakListAssetItem
{
	FString AbsPath;
	FString MountPak;
};

FPakListAssetItem BreakPakListLine(const FString& PakListLine)
{
	FPakListAssetItem result;
	TArray<FString> AssetPakCmd = UKismetStringLibrary::ParseIntoArray(PakListLine,TEXT("\" "));

	FString AssetAbsPath = AssetPakCmd[0];
	FString AssetMountPath = AssetPakCmd[1];
	result.AbsPath = NormalizePaklistPath(AssetAbsPath);
	result.MountPak = NormalizePaklistPath(AssetMountPath);
	return result;
}

void FReleasePaklistParser::Parser(TSharedPtr<FReleaseParserConf> ParserConf, EHashCalculator HashCalculator)
{
	result.Platform = ParserConf->TargetPlatform;
	FReleasePakListConf* Conf = (FReleasePakListConf*)ParserConf.Get();
	
	for(const auto& ResponseFile:Conf->PakResponseFiles)
	{
		if (FPaths::FileExists(ResponseFile))
		{
			TArray<FString> PakListContent;
			if (FFileHelper::LoadFileToStringArray(PakListContent, *ResponseFile))
			{
				for(const auto& PakListLine:PakListContent)
				{
					FPakListAssetItem LineItem = BreakPakListLine(PakListLine);
					FString Extersion = FPaths::GetExtension(LineItem.MountPak,true);
					bool bIsWPFiles = false;
				#if UE_VERSION_NEWER_THAN(5,0,0)
					bIsWPFiles = LineItem.MountPak.Contains(TEXT("/_Generated_/"));
				#endif
					if(!bIsWPFiles && UFlibPatchParserHelper::GetCookedUassetExtensions().Contains(Extersion))
					{
						if(UFlibPatchParserHelper::GetUnCookUassetExtensions().Contains(Extersion))
						{
							// is uasset
							FPatcherSpecifyAsset SpecifyAsset;
							FString AbsFile = UFlibPatchParserHelper::AssetMountPathToAbs(LineItem.MountPak);
							FString LongPackageName;
							if(FPackageName::TryConvertFilenameToLongPackageName(AbsFile,LongPackageName))
							{
								FString PackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(LongPackageName);
								SpecifyAsset.Asset = FSoftObjectPath{PackagePath};
								SpecifyAsset.bAnalysisAssetDependencies = false;
								SpecifyAsset.AssetRegistryDependencyTypes = {EAssetRegistryDependencyTypeEx::None};
								if(SpecifyAsset.Asset.IsValid())
									result.Assets.Add(SpecifyAsset);
							}
							else
							{
								UE_LOG(LogHotPatcher,Warning,TEXT("%s conv to abs path failed!"),*LineItem.MountPak);
							}
						}
					}
					else
					{
						// is not-uasset
						FExternFileInfo ExFile;
						ExFile.SetFilePath(LineItem.AbsPath);
						ExFile.MountPath = LineItem.MountPak;
						ExFile.GenerateFileHash(HashCalculator);
						result.ExternFiles.AddUnique(ExFile);
					}
				}
			}
		}
	}
}
