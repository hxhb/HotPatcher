#include "CreatePatch/FExportReleaseSettings.h"


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
			TArray<FString> PakFiles;
			for(const auto& PakFile:PlatformPakList.PakLists)
			{
				PakFiles.AddUnique(PakFile.FilePath);
			}
			PlatformAssets.Add(PlatformPakListParser(PlatformPakList.TargetPlatform,PakFiles));
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
		FPlatformPakAssets PlatformAssets = PlatformPakListParser(ETargetPlatform::AllPlatforms,InPaklistFile);
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
	
	FPlatformPakAssets FExportReleaseSettings::PlatformPakListParser(const ETargetPlatform Platform, const TArray<FString>& InPaklistFiles)
	{
		FPlatformPakAssets result;
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

					TMap<FString, FString> ReceiveModuleMap;
					UFLibAssetManageHelperEx::GetAllEnabledModuleName(ReceiveModuleMap);
					auto ParseUassetLambda = [&RemoveDoubleQuoteLambda,&ReceiveModuleMap](const FString& InAsset)->FPatcherSpecifyAsset
					{
						FPatcherSpecifyAsset result;
						result.bAnalysisAssetDependencies = false;
			
						
						TArray<FString> AssetPakCmd = UKismetStringLibrary::ParseIntoArray(InAsset,TEXT("\" "));

						FString AssetAbsPath = AssetPakCmd[0];
						FString AssetMountPath = AssetPakCmd[1];
						AssetAbsPath = RemoveDoubleQuoteLambda(AssetAbsPath);
						AssetMountPath = RemoveDoubleQuoteLambda(AssetMountPath);
							
						FString LongPackageName = AssetMountPath;

						LongPackageName.RemoveFromStart(TEXT("../../.."));
						LongPackageName.RemoveFromEnd(TEXT(".uasset"));
							
						// LongPackageName = LongPackageName.Replace(TEXT("/Content"), TEXT(""));
							
						FString ModuleName = LongPackageName;
						{
							ModuleName.RemoveAt(0);
							int32 Index;
							if (ModuleName.FindChar('/', Index))
							{
								ModuleName = ModuleName.Left(Index);
							}
						}

						if (ModuleName.Equals(FApp::GetProjectName()))
						{
							LongPackageName.RemoveFromStart(TEXT("/") + ModuleName);
							LongPackageName = TEXT("/Game") + LongPackageName;
						}

						if (LongPackageName.Contains(TEXT("/Plugins/")))
						{
							TArray<FString> ModuleKeys;
							ReceiveModuleMap.GetKeys(ModuleKeys);

							TArray<FString> IgnoreModules = { TEXT("Game"),TEXT("Engine") };

							for (const auto& IgnoreModule : IgnoreModules)
							{
								ModuleKeys.Remove(IgnoreModule);
							}

							for (const auto& Module : ModuleKeys)
							{
								FString FindStr = TEXT("/") + Module + TEXT("/");
								if (LongPackageName.Contains(FindStr,ESearchCase::IgnoreCase))
								{
									int32 PluginModuleIndex = LongPackageName.Find(FindStr,ESearchCase::IgnoreCase,ESearchDir::FromEnd);

									LongPackageName = LongPackageName.Right(LongPackageName.Len()- PluginModuleIndex-FindStr.Len());
									LongPackageName = TEXT("/") + Module + TEXT("/") + LongPackageName;
									break;
								}
							}
						}

						int32 ContentPoint = LongPackageName.Find(TEXT("/Content"));
						if(ContentPoint != INDEX_NONE)
						{
							LongPackageName = FPaths::Combine(LongPackageName.Left(ContentPoint), LongPackageName.Right(LongPackageName.Len() - ContentPoint - 8));
						}

						FString LongPackagePath;
						if(UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(LongPackageName, LongPackagePath))
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
					TArray<FString> UAssetFormats = { TEXT(".uasset"),TEXT(".ubulk"),TEXT(".uexp"),TEXT(".umap"),TEXT(".ufont") };
					auto IsUAssetLambda = [&UAssetFormats](const FString& InStr)->bool
					{
						bool bResult = false;
						for (const auto& Format:UAssetFormats)
						{
							if (InStr.Contains(Format))
							{
								bResult = true;
								break;
							}
						}
						return bResult;
					};

					for (const auto& FileItem : PakListContent)
					{
						if (IsUAssetLambda(FileItem))
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
		
		return result;
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
