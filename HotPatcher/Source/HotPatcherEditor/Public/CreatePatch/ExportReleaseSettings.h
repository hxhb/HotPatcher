// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// project header
#include "FPatcherSpecifyAsset.h"

// engine header
#include "Misc/FileHelper.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "ExportReleaseSettings.generated.h"

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
UCLASS()
class UExportReleaseSettings : public UObject
{
	GENERATED_BODY()
public:
	UExportReleaseSettings()
	{
		AssetRegistryDependencyTypes.Add(EAssetRegistryDependencyTypeEx::Packages);
	}

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
	{
		if (PropertyChangedEvent.Property && PropertyChangedEvent.MemberProperty->GetName() == TEXT("PakListFile"))
		{
			FString PakListAbsPath = FPaths::ConvertRelativePathToFull(PakListFile.FilePath);
			ParseByPaklist(this, PakListAbsPath);
			
		}
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}

	virtual bool ParseByPaklist(UExportReleaseSettings* InReleaseSetting,const FString& InPaklistFile)
	{
		if (FPaths::FileExists(InPaklistFile))
		{
			TArray<FString> PakListContent;
			if (FFileHelper::LoadFileToStringArray(PakListContent, *InPaklistFile))
			{
				auto ParseUassetLambda = [](const FString& InAsset)->FPatcherSpecifyAsset
				{
					FPatcherSpecifyAsset result;
					result.bAnalysisAssetDependencies = false;
		
					FString LongPackagePath;
					{
						int32 breakPoint = InAsset.Find(TEXT("\" \""));
						FString AssetAbsPath = InAsset.Left(breakPoint);
						AssetAbsPath.RemoveAt(0);
						FPaths::MakeStandardFilename(AssetAbsPath);

						FString LongPackageName = InAsset.Right(InAsset.Len() - breakPoint - 3);
						LongPackageName.RemoveFromStart(TEXT("../../.."));
						LongPackageName.RemoveFromEnd(TEXT(".uasset\""));
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

					UE_LOG(LogTemp, Log, TEXT("UEAsset: Str: %s LongPackagePath %s"),*InAsset,*LongPackagePath);
					result.Asset = LongPackagePath;
					return result;
				};
				auto ParseNoAssetFileLambda = [](const FString& InAsset)->FExternAssetFileInfo
				{
					FExternAssetFileInfo result;
					int32 breakPoint = InAsset.Find(TEXT("\" \""));

					auto RemoveDoubleQuoteLambda = [](const FString& InStr)->FString
					{
						FString result = InStr;
						result.RemoveAt(0);
						result.RemoveAt(result.Len() - 1);
						return result;
					};
					result.FilePath.FilePath = RemoveDoubleQuoteLambda(InAsset.Left(breakPoint+1));
					result.MountPath = RemoveDoubleQuoteLambda(InAsset.Right(InAsset.Len() - breakPoint - 2));
					result.GenerateFileHash();
					UE_LOG(LogTemp, Log, TEXT("NoAsset: BreakPoint:%d,left:%s,Right:%s"), breakPoint, *result.FilePath.FilePath, *result.MountPath);
					return result;
				};
				TArray<FString> UAssetFormats = { TEXT(".uasset"),TEXT(".ubulk"),TEXT(".uexp"),TEXT(".umap") };
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
							InReleaseSetting->AddSpecifyAsset(Asset);
						}
						continue;
					}
					else
					{
						FExternAssetFileInfo ExFile = ParseNoAssetFileLambda(FileItem);
						InReleaseSetting->AddExternFileToPak.Add(ExFile);
					}
					
				}
			}

		}
		return true;
	}
	FORCEINLINE static UExportReleaseSettings* Get()
	{
		static bool bInitialized = false;
		// This is a singleton, use default object
		UExportReleaseSettings* DefaultSettings = GetMutableDefault<UExportReleaseSettings>();
		DefaultSettings->AddToRoot();
		if (!bInitialized)
		{
			bInitialized = true;
		}

		return DefaultSettings;
	}

	FORCEINLINE bool SerializeReleaseConfigToString(FString& OutJsonString)
	{
		TSharedPtr<FJsonObject> ReleaseConfigJsonObject = MakeShareable(new FJsonObject);
		SerializeReleaseConfigToJsonObject(ReleaseConfigJsonObject);

		auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutJsonString);
		return FJsonSerializer::Serialize(ReleaseConfigJsonObject.ToSharedRef(), JsonWriter);
	}

	FORCEINLINE bool SerializeReleaseConfigToJsonObject(TSharedPtr<FJsonObject>& OutJsonObject)
	{
		return UFlibHotPatcherEditorHelper::SerializeReleaseConfigToJsonObject(this, OutJsonObject);
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

	FORCEINLINE TArray<FExternAssetFileInfo> GetAllExternFiles(bool InGeneratedHash = false)const
	{
		TArray<FExternAssetFileInfo> AllExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(GetAddExternDirectory());

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

	FORCEINLINE TArray<FExternAssetFileInfo> GetAddExternFiles()const { return AddExternFileToPak; }
	FORCEINLINE TArray<FExternDirectoryInfo> GetAddExternDirectory()const { return AddExternDirectoryToPak; }

	FORCEINLINE bool IsByPakList()const { return ByPakList; }
	FORCEINLINE FFilePath GetPakListFile()const { return PakListFile; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Version")
		FString VersionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version")
		bool ByPakList;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Version", meta = (RelativeToGameContentDir, EditCondition = "ByPakList"))
		FFilePath PakListFile;
	/** You can use copied asset string reference here, e.g. World'/Game/NewMap.NewMap'*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "ReleaseSetting|Asset Filters",meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIncludeFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Asset Filters", meta = (RelativeToGameContentDir, LongPackageName))
		TArray<FDirectoryPath> AssetIgnoreFilters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Asset Filters")
		bool bAnalysisFilterDependencies=true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Asset Filters",meta=(EditCondition="bAnalysisFilterDependencies"))
		TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDependencyTypes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Asset Filters")
		bool bIncludeHasRefAssetsOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Specify Assets")
		TArray<FPatcherSpecifyAsset> IncludeSpecifyAssets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Extern Files")
		TArray<FExternAssetFileInfo> AddExternFileToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReleaseSetting|Extern Files")
		TArray<FExternDirectoryInfo> AddExternDirectoryToPak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveAssetRelatedInfo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveTo")
		bool bSaveReleaseConfig;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "SaveTo")
		FDirectoryPath SavePath;
};
