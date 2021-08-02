#include "CreatePatch/FExportReleaseSettings.h"
#include "Kismet/KismetSystemLibrary.h"

FExportReleaseSettings::FExportReleaseSettings()
	{
		AssetRegistryDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);
	}
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
		TArray<FPlatformPakAssets> PlatformAssets;
		for(const auto& PlatformPakList:GetPlatformsPakListFiles())
		{
			TArray<FString> PakFilesInResponse;
			for(const auto& PakFile:PlatformPakList.PakResponseFiles)
			{
				PakFilesInResponse.AddUnique(PakFile.FilePath);
			}
			FPlatformPakAssets currentPak;
			PlatformPakListParser(PlatformPakList.TargetPlatform,PakFilesInResponse,currentPak);
#if WITH_EDITOR
			TArray<FString> PakFiles;
			for(const auto& pakFile:PlatformPakList.PakFiles)
			{
				PakFiles.Append(UFlibPakHelper::GetPakFileList(pakFile.FilePath,PlatformPakList.AESKey));
			}
			
			PlatformPakFileParser(PlatformPakList.TargetPlatform,PakFiles,currentPak);
#endif
			PlatformAssets.Add(currentPak);
		}

		PlatformAssets.Sort([](const FPlatformPakAssets& l,const FPlatformPakAssets& r)->bool{return l.Assets.Num()<r.Assets.Num();});
		
		for(const auto& Asset:PlatformAssets[0].Assets)
        {
			AddSpecifyAsset(Asset);
        }

		PlatformAssets.Sort([](const FPlatformPakAssets& l,const FPlatformPakAssets& r)->bool{return l.ExternFiles.Num()<r.ExternFiles.Num();});

		// 分析全平台都包含的外部文件添加至AllPlatform
		if(PlatformAssets.Num() > 1)
		{
			FPlatformPakAssets AllPlatform;
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
		IncludeSpecifyAssets.Empty();
	}
	
	// for DetailView property change property
	void FExportReleaseSettings::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
	{
		// PostEditChangeProperty(PropertyChangedEvent);
	}
	
	void FExportReleaseSettings::PostEditChangeProperty(const FPropertyChangedEvent& PropertyChangedEvent)
	{
		
	}

	bool FExportReleaseSettings::ParseByPaklist(FExportReleaseSettings* InReleaseSetting,const TArray<FString>& InPaklistFile)
	{
		FPlatformPakAssets PlatformAssets;
		PlatformPakListParser(ETargetPlatform::AllPlatforms,InPaklistFile,PlatformAssets);
		for(const auto& Asset:PlatformAssets.Assets)
		{
			InReleaseSetting->AddSpecifyAsset(Asset);
		}
		for(const auto& File:PlatformAssets.ExternFiles)
		{
			InReleaseSetting->AddExternFileToPak.AddUnique(File);
		}
		return true;
	}
	
	bool FExportReleaseSettings::PlatformPakListParser(const ETargetPlatform Platform, const TArray<FString>& InPaklistFiles,FPlatformPakAssets& result)
	{
		result.Platform = Platform;
		for(auto& File:InPaklistFiles)
		{
			if (FPaths::FileExists(File))
			{
				TArray<FString> PakListContent;
				if (FFileHelper::LoadFileToStringArray(PakListContent, *File))
				{
					auto RemoveDoubleQuoteLambda = [](const FString& InStr)->FString
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

					auto ParseUassetLambda = [&RemoveDoubleQuoteLambda](const FString& InAsset)->FPatcherSpecifyAsset
					{
						FPatcherSpecifyAsset result;
						result.bAnalysisAssetDependencies = false;
			
						
						TArray<FString> AssetPakCmd = UKismetStringLibrary::ParseIntoArray(InAsset,TEXT("\" "));

						FString AssetAbsPath = AssetPakCmd[0];
						FString AssetMountPath = AssetPakCmd[1];
						AssetAbsPath = RemoveDoubleQuoteLambda(AssetAbsPath);
						AssetMountPath = RemoveDoubleQuoteLambda(AssetMountPath);

						FString LongPackagePath = UFlibPatchParserHelper::UAssetMountPathToPackagePath(AssetMountPath);
						if(!LongPackagePath.IsEmpty())
						{
							result.Asset = LongPackagePath;
						}
						else
						{
							UE_LOG(LogHotPatcher,Warning,TEXT("Paklist Item: %s is invalid uasset!!!"),*InAsset);
						}
						return result;
						// UE_LOG(LogHotPatcher, Log, TEXT("UEAsset: Str: %s LongPackagePath %s"),*InAsset,*LongPackagePath);
					};
					auto ParseNoAssetFileLambda = [&RemoveDoubleQuoteLambda](const FString& InAsset)->FExternFileInfo
					{
						FExternFileInfo result;
						
						TArray<FString> AssetPakCmd = UKismetStringLibrary::ParseIntoArray(InAsset,TEXT("\" "));

						FString AssetAbsPath = AssetPakCmd[0];
						FString AssetMountPath = AssetPakCmd[1];

						result.FilePath.FilePath = RemoveDoubleQuoteLambda(AssetAbsPath);
						result.MountPath = RemoveDoubleQuoteLambda(AssetMountPath);

						result.GenerateFileHash();
						// UE_LOG(LogHotPatcher, Log, TEXT("NoAsset: left:%s,Right:%s"), *result.FilePath.FilePath, *result.MountPath);
						return result;
					};

					for (const auto& FileItem : PakListContent)
					{
						if (UFlibPatchParserHelper::IsUasset(FileItem))
						{
							if (FileItem.Contains(TEXT(".uasset")))
							{
								FPatcherSpecifyAsset Asset = ParseUassetLambda(FileItem);
								if(Asset.Asset.IsValid())
									result.Assets.AddUnique(Asset);
							}
							continue;
						}
						else
						{
							FExternFileInfo ExFile = ParseNoAssetFileLambda(FileItem);
							result.ExternFiles.AddUnique(ExFile);
						}					
					}
				}

			}
		}
		return true;
	}

bool FExportReleaseSettings::PlatformPakFileParser(const ETargetPlatform Platform, const TArray<FString>& InMountFilePaths, FPlatformPakAssets& result)
{
	result.Platform = Platform;

	if(!InMountFilePaths.Num())
		return false;

	for(const auto& MountFile:InMountFilePaths)
	{
		if(UFlibPatchParserHelper::IsUasset(MountFile))
		{
			// is uasset
			FString LongPacakgePath = UFlibPatchParserHelper::UAssetMountPathToPackagePath(MountFile);
			FPatcherSpecifyAsset currentAsset;
			currentAsset.Asset = FSoftObjectPath(LongPacakgePath);
			currentAsset.bAnalysisAssetDependencies = false;
			if(currentAsset.Asset.IsValid())
			{
				result.Assets.Add(currentAsset);
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
			
			UFLibAssetManageHelperEx::GetModuleNameByRelativePath(RelativePath,ModuleName);
			UFLibAssetManageHelperEx::GetEnableModuleAbsDir(ModuleName,ModuleAbsPath);
			RelativePath.RemoveFromStart(ModuleName);
			FString FinalFilePath = FPaths::Combine(ModuleAbsPath,RelativePath);
			FPaths::NormalizeFilename(FinalFilePath);
			currentFile.FilePath.FilePath = FinalFilePath;
			currentFile.MountPath = MountFile;
			result.ExternFiles.AddUnique(currentFile);
		}
	}
	return true;
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
TArray<FString> FExportReleaseSettings::GetAssetIncludeFiltersPaths()const
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
TArray<FString> FExportReleaseSettings::GetAssetIgnoreFiltersPaths()const
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

TArray<FExternFileInfo> FExportReleaseSettings::GetAllExternFiles(bool InGeneratedHash)const
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
