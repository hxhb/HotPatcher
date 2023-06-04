#include "CreatePatch/FExportReleaseSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ReleaseParser/FReleasePakParser.h"
#include "ReleaseParser/FReleasePaklistParser.h"

FExportReleaseSettings::FExportReleaseSettings(){}
FExportReleaseSettings::~FExportReleaseSettings(){};

void FExportReleaseSettings::Init()
{
	Super::Init();
}
		void FExportReleaseSettings::ImportPakLists()
	{
		UE_LOG(LogHotPatcher,Log,TEXT("FExportReleaseSettings::ImportPakList"));
		
		if(!GetPlatformsPakListFiles().Num())
		{
			return;
		}
		TArray<FReleaseParserResult> PlatformAssets;
		for(const auto& PlatformPakList:GetPlatformsPakListFiles())
		{
			TSharedPtr<FReleasePakListConf> PakListConf = MakeShareable(new FReleasePakListConf);
			PakListConf->TargetPlatform = PlatformPakList.TargetPlatform;
			for(const auto& PakFile:PlatformPakList.PakResponseFiles)
			{
				PakListConf->PakResponseFiles.AddUnique(PakFile.FilePath);
			}
			if(!!PakListConf->PakResponseFiles.Num())
			{
				FReleasePaklistParser PakListParser;
				PakListParser.Parser(PakListConf,GetHashCalculator());
				PlatformAssets.Add(PakListParser.GetParserResult());
			}

			TSharedPtr<FReleasePakFilesConf> PakFileConf = MakeShareable(new FReleasePakFilesConf);
			PakFileConf->TargetPlatform = PlatformPakList.TargetPlatform;
			for(const auto& PakFile:PlatformPakList.PakFiles)
			{
				PakFileConf->PakFiles.AddUnique(FPaths::ConvertRelativePathToFull(PakFile.FilePath));
			}
			PakFileConf->AESKey = PlatformPakList.AESKey;
			if(!!PakFileConf->PakFiles.Num())
			{
				FReleasePakParser PakFileParser;
				PakFileParser.Parser(PakFileConf,GetHashCalculator());
				PlatformAssets.Add(PakFileParser.GetParserResult());
			}
		}

		PlatformAssets.Sort([](const FReleaseParserResult& l,const FReleaseParserResult& r)->bool{return l.Assets.Num()<r.Assets.Num();});
		
		for(auto& PlatformAsset:PlatformAssets)
		{
			for(const auto& Asset:PlatformAsset.Assets)
			{
				AddSpecifyAsset(Asset);
			}
		}

		PlatformAssets.Sort([](const FReleaseParserResult& l,const FReleaseParserResult& r)->bool{return l.ExternFiles.Num()<r.ExternFiles.Num();});

		// 分析全平台都包含的外部文件添加至AllPlatform
		if(PlatformAssets.Num() > 1)
		{
			FReleaseParserResult AllPlatform;
			AllPlatform.Platform = ETargetPlatform::AllPlatforms;
			for(int32 FileIndex=0;FileIndex<PlatformAssets[0].ExternFiles.Num();)
			{
				FExternFileInfo& MinPlatformFileItem = PlatformAssets[0].ExternFiles[FileIndex];
				bool IsAllPlatformFile = true;
				for(int32 Internalindex =1;Internalindex<PlatformAssets.Num();++Internalindex)
				{
					int32 FindIndex = PlatformAssets[Internalindex].ExternFiles.FindLastByPredicate([&MinPlatformFileItem](const FExternFileInfo& file)
                    {
                        return MinPlatformFileItem.IsSameMount(file);
                    });
				
					if(FindIndex != INDEX_NONE)
					{
						PlatformAssets[Internalindex].ExternFiles.RemoveAt(FindIndex);				
					}
					else
					{
						IsAllPlatformFile = false;
						break;
					}
				}
				if(IsAllPlatformFile)
				{
					AllPlatform.ExternFiles.AddUnique(MinPlatformFileItem);
					PlatformAssets[0].ExternFiles.RemoveAt(FileIndex);
					continue;
				}
				++FileIndex;
			}
			PlatformAssets.Add(AllPlatform);
		}
		
		// for not-uasset file
		for(const auto& Platform:PlatformAssets)
		{
			auto CheckIsExisitPlatform = [this](ETargetPlatform InPlatform)->bool
			{
				bool result = false;
				for(auto& ExistPlatform:AddExternAssetsToPlatform)
				{
					if(ExistPlatform.TargetPlatform == InPlatform)
					{
						result = true;
						break;
					}
				}
				return result;
			};

			if(CheckIsExisitPlatform(Platform.Platform))
			{
				for(auto& ExistPlatform:AddExternAssetsToPlatform)
				{
					if(ExistPlatform.TargetPlatform == Platform.Platform)
					{
						ExistPlatform.AddExternFileToPak.Append(Platform.ExternFiles);
					}
				}
			}
			else
			{
				FPlatformExternAssets NewPlatform;
				NewPlatform.TargetPlatform = Platform.Platform;
				NewPlatform.AddExternFileToPak = Platform.ExternFiles;
				AddExternAssetsToPlatform.Add(NewPlatform);
			}	
		}
	}

	
void FExportReleaseSettings::ClearImportedPakList()
{
	UE_LOG(LogHotPatcher,Log,TEXT("FExportReleaseSettings::ClearImportedPakList"));
	AddExternAssetsToPlatform.Empty();
	GetAssetScanConfigRef().IncludeSpecifyAssets.Empty();
}
	
// for DetailView property change property
void FExportReleaseSettings::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	// PostEditChangeProperty(PropertyChangedEvent);
}
	
void FExportReleaseSettings::PostEditChangeProperty(const FPropertyChangedEvent& PropertyChangedEvent)
{
		
}

FExportReleaseSettings* FExportReleaseSettings::Get()
{
	static FExportReleaseSettings StaticIns;
	return &StaticIns;
}

FString FExportReleaseSettings::GetVersionId()const
{
	return VersionId;
}

