// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
#include "FPatcherSpecifyAsset.h"
#include "FExternFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPlatformExternAssets.h"
#include "HotPatcherLog.h"
#include "CreatePatch/HotPatcherSettingBase.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
// engine header
#include "Misc/FileHelper.h"
#include "CoreMinimal.h"

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetStringLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "FExportReleaseSettings.generated.h"


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPlatformPakListFiles
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	ETargetPlatform TargetPlatform = ETargetPlatform::None;
	UPROPERTY(EditAnywhere)
	FFilePath PakList;
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FPlatformPakAssets
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere)
		ETargetPlatform Platform;
	UPROPERTY(EditAnywhere)
		TArray<FPatcherSpecifyAsset> Assets;
	UPROPERTY(EditAnywhere)
		TArray<FExternFileInfo> ExternFiles;
};

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FExportReleaseSettings:public FHotPatcherSettingBase
{
	GENERATED_USTRUCT_BODY()
public:
	FExportReleaseSettings()
	{
		AssetRegistryDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);
	}
	virtual ~FExportReleaseSettings(){};

	virtual void ImportPakLists()
	{
		UE_LOG(LogHotPatcher,Log,TEXT("FExportReleaseSettings::ImportPakList"));
		
		if(!GetPlatformsPakListFiles().Num())
		{
			return;
		}
		TArray<FPlatformPakAssets> PlatformAssets;
		for(const auto& PlatformPakList:GetPlatformsPakListFiles())
		{
			PlatformAssets.Add(PlatformPakListParser(PlatformPakList.TargetPlatform,PlatformPakList.PakList.FilePath));
		}
		
		PlatformAssets.Sort([](const FPlatformPakAssets& l,const FPlatformPakAssets& r)->bool{return l.Assets.Num()<r.Assets.Num();});
		
		for(const auto& Asset:PlatformAssets[0].Assets)
        {
			AddSpecifyAsset(Asset);
        }

		PlatformAssets.Sort([](const FPlatformPakAssets& l,const FPlatformPakAssets& r)->bool{return l.ExternFiles.Num()<r.ExternFiles.Num();});

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
					return MinPlatformFileItem.IsAbsSame(file);
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
	
	virtual void ClearImportedPakList()
	{
		UE_LOG(LogHotPatcher,Log,TEXT("FExportReleaseSettings::ClearImportedPakList"));
		AddExternAssetsToPlatform.Empty();
		IncludeSpecifyAssets.Empty();
	}
	
	// for DetailView property change property
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
	{
		// PostEditChangeProperty(PropertyChangedEvent);
	}
	
	virtual void PostEditChangeProperty(const FPropertyChangedEvent& PropertyChangedEvent)
	{
		if (PropertyChangedEvent.Property && PropertyChangedEvent.MemberProperty->GetName() == TEXT("PakListFile"))
		{
			FString PakListAbsPath = FPaths::ConvertRelativePathToFull(PakListFile.FilePath);
			ParseByPaklist(this, PakListAbsPath);
			
		}
		// Super::PostEditChangeProperty(PropertyChangedEvent);
	}

	virtual bool ParseByPaklist(FExportReleaseSettings* InReleaseSetting,const FString& InPaklistFile)
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
	
	virtual FPlatformPakAssets PlatformPakListParser(const ETargetPlatform Platform,const FString& InPaklistFile)
	{
		FPlatformPakAssets result;
		result.Platform = Platform;
		if (FPaths::FileExists(InPaklistFile))
		{
			TArray<FString> PakListContent;
			if (FFileHelper::LoadFileToStringArray(PakListContent, *InPaklistFile))
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
		
					FString LongPackagePath;
					{
						TArray<FString> AssetPakCmd = UKismetStringLibrary::ParseIntoArray(InAsset,TEXT("\" "));

						FString AssetAbsPath = AssetPakCmd[0];
						FString AssetMountPath = AssetPakCmd[1];
						AssetAbsPath = RemoveDoubleQuoteLambda(AssetAbsPath);
						AssetMountPath = RemoveDoubleQuoteLambda(AssetMountPath);
						
						FString LongPackageName = AssetMountPath;

						LongPackageName.RemoveFromStart(TEXT("../../.."));
						LongPackageName.RemoveFromEnd(TEXT(".uasset"));
						
						LongPackageName.Replace(TEXT("/Content"), TEXT(""));
						
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
							TMap<FString, FString> ReceiveModuleMap;
							UFLibAssetManageHelperEx::GetAllEnabledModuleName(ReceiveModuleMap);
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
								if (LongPackageName.Contains(FindStr))
								{
									int32 PluginModuleIndex = LongPackageName.Find(FindStr,ESearchCase::CaseSensitive,ESearchDir::FromEnd);

									LongPackageName = LongPackageName.Right(LongPackageName.Len()- PluginModuleIndex-FindStr.Len());
									LongPackageName = TEXT("/") + Module + TEXT("/") + LongPackageName;
									break;
								}
							}
						}

						int32 ContentPoint = LongPackageName.Find(TEXT("/Content"));
						LongPackageName = FPaths::Combine(LongPackageName.Left(ContentPoint), LongPackageName.Right(LongPackageName.Len() - ContentPoint - 8));
						UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(LongPackageName, LongPackagePath);
					}

					// UE_LOG(LogHotPatcher, Log, TEXT("UEAsset: Str: %s LongPackagePath %s"),*InAsset,*LongPackagePath);
					result.Asset = LongPackagePath;
					return result;
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
		return result;
	}
	
	FORCEINLINE static FExportReleaseSettings* Get()
	{
		static FExportReleaseSettings StaticIns;
		return &StaticIns;
	}

	FORCEINLINE FString GetVersionId()const
	{
		return VersionId;
	}
	FORCEINLINE TArray<FString> GetAssetIncludeFiltersPaths()const
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
	FORCEINLINE TArray<FString> GetAssetIgnoreFiltersPaths()const
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

	FORCEINLINE TArray<FExternFileInfo> GetAllExternFiles(bool InGeneratedHash=false)const
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
	
	FORCEINLINE TArray<FExternFileInfo> GetAllExternFilesByPlatform(ETargetPlatform InTargetPlatform,bool InGeneratedHash = false)const
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
	
	FORCEINLINE TMap<ETargetPlatform,FPlatformExternFiles> GetAllPlatfotmExternFiles(bool InGeneratedHash = false)const
	{
		TMap<ETargetPlatform,FPlatformExternFiles> result;
	
		for(const auto& Platform:GetAddExternAssetsToPlatform())
		{
			FPlatformExternFiles PlatformIns(Platform.TargetPlatform,GetAllExternFilesByPlatform(Platform.TargetPlatform,InGeneratedHash));
			result.Add(Platform.TargetPlatform,PlatformIns);
		}
		return result;
	}

	FORCEINLINE TArray<FPlatformExternAssets> GetAddExternAssetsToPlatform()const{return AddExternAssetsToPlatform;}
	
	FORCEINLINE TArray<FExternFileInfo> GetAddExternFilesByPlatform(ETargetPlatform InTargetPlatform)const
	{
		for(const auto& Platform:GetAddExternAssetsToPlatform())
		{
			if (Platform.TargetPlatform == InTargetPlatform)
			{
				return Platform.AddExternFileToPak;
			}
		}

		return TArray<FExternFileInfo>{};
	}
	FORCEINLINE TArray<FExternDirectoryInfo> GetAddExternDirectoryByPlatform(ETargetPlatform InTargetPlatform)const
	{
		for(const auto& Platform:GetAddExternAssetsToPlatform())
		{
			if (Platform.TargetPlatform == InTargetPlatform)
			{
				return Platform.AddExternDirectoryToPak;
			}
		}

		return TArray<FExternDirectoryInfo>{};
	}
	

	
	FORCEINLINE FString GetSavePath()const{return SavePath.Path;}
	FORCEINLINE bool IsSaveConfig()const {return bSaveReleaseConfig;}
	FORCEINLINE bool IsSaveAssetRelatedInfo()const { return bSaveAssetRelatedInfo; }
	FORCEINLINE bool IsIncludeHasRefAssetsOnly()const { return bIncludeHasRefAssetsOnly; }
	FORCEINLINE bool IsAnalysisFilterDependencies()const { return bAnalysisFilterDependencies; }
	FORCEINLINE TArray<EAssetRegistryDependencyTypeEx> GetAssetRegistryDependencyTypes()const { return AssetRegistryDependencyTypes; }
	FORCEINLINE TArray<FPatcherSpecifyAsset> GetSpecifyAssets()const { return IncludeSpecifyAssets; }
	FORCEINLINE bool AddSpecifyAsset(FPatcherSpecifyAsset const& InAsset)
	{
		IncludeSpecifyAssets.AddUnique(InAsset);
		return true;
	}

	FORCEINLINE TArray<FExternFileInfo> GetAddExternFiles()const { return AddExternFileToPak; }
	FORCEINLINE TArray<FExternDirectoryInfo> GetAddExternDirectory()const { return AddExternDirectoryToPak; }

	FORCEINLINE bool IsByPakList()const { return ByPakList; }
	FORCEINLINE FFilePath GetPakListFile()const { return PakListFile; }
	FORCEINLINE TArray<FPlatformPakListFiles> GetPlatformsPakListFiles()const {return PlatformsPakListFiles;}
	
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Version")
		FString VersionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version")
		bool ByPakList = false;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version", meta = (RelativeToGameContentDir, EditCondition = "ByPakList"))
		FFilePath PakListFile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version", meta = (RelativeToGameContentDir, EditCondition = "ByPakList"))
		TArray<FPlatformPakListFiles> PlatformsPakListFiles;
	
	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "AssetFilters",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters")
		bool bAnalysisFilterDependencies = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters",meta=(EditCondition="bAnalysisFilterDependencies"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssetFilters")
		bool bIncludeHasRefAssetsOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpecifyAssets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExternFiles")
		TArray<FExternFileInfo> AddExternFileToPak;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExternFiles")
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExternFiles")
		TArray<FPlatformExternAssets> AddExternAssetsToPlatform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveAssetRelatedInfo = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveReleaseConfig = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		FDirectoryPath SavePath;
};

