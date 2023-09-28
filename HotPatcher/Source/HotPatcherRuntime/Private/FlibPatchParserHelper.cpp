// Fill out your copyright notice in the Description page of Project Settings.

// project header
#include "FlibPatchParserHelper.h"
#include "FlibAssetManageHelper.h"
#include "BaseTypes/AssetManager/FFileArrayDirectoryVisitor.hpp"
#include "FlibAssetManageHelper.h"
#include "FlibAssetManageHelper.h"
#include "HotPatcherLog.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "DependenciesParser/FDefaultAssetDependenciesParser.h"

// engine header
#include "Misc/App.h"
#include "Misc/SecureHash.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "Engine/EngineTypes.h"
#include "JsonObjectConverter.h"
#include "AssetRegistryState.h"
#include "FPackageTracker.h"
#include "HotPatcherRuntime.h"
#include "Async/ParallelFor.h"
#include "CreatePatch/TimeRecorder.h"
#include "Misc/Paths.h"
#include "Kismet/KismetStringLibrary.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Serialization/LargeMemoryReader.h"

TArray<FString> UFlibPatchParserHelper::GetAvailableMaps(FString GameName, bool IncludeEngineMaps, bool IncludePluginMaps, bool Sorted)
{
	const FString WildCard = FString::Printf(TEXT("*%s"), *FPackageName::GetMapPackageExtension());
	TArray<FString> AllMaps;
	TMap<FString,FString> AllEnableModules;

	UFlibAssetManageHelper::GetAllEnabledModuleName(AllEnableModules);

	auto ScanMapsByModuleName = [WildCard,&AllMaps](const FString& InModuleBaseDir)
	{
		TArray<FString> OutMaps;
		FString ModuleContentAbsPath= FPaths::ConvertRelativePathToFull(FPaths::Combine(*InModuleBaseDir, TEXT("Content")));
		IFileManager::Get().FindFilesRecursive(OutMaps, *ModuleContentAbsPath, *WildCard, true, false);
		
		for (const auto& MapPath : OutMaps)
		{
			AllMaps.Add(FPaths::GetBaseFilename(MapPath));
		}
	};

	ScanMapsByModuleName(AllEnableModules[TEXT("Game")]);
	if (IncludeEngineMaps)
	{
		ScanMapsByModuleName(AllEnableModules[TEXT("Engine")]);
	}
	if (IncludePluginMaps)
	{
		TArray<FString> AllModuleNames;
		AllEnableModules.GetKeys(AllModuleNames);

		for (const auto& ModuleName : AllModuleNames)
		{
			if (!ModuleName.Equals(TEXT("Game")) && !ModuleName.Equals(TEXT("Engine")))
			{
				ScanMapsByModuleName(AllEnableModules[ModuleName]);
			}
		}
	}
	if (Sorted)
	{
		AllMaps.Sort();
	}

	return AllMaps;
}

FString UFlibPatchParserHelper::GetProjectName()
{
	return FApp::GetProjectName();
}


FString UFlibPatchParserHelper::GetProjectFilePath()
{
	FString ProjectFilePath;
	{
		FString ProjectPath = UKismetSystemLibrary::GetProjectDirectory();
		FString ProjectName = FString(FApp::GetProjectName()).Append(TEXT(".uproject"));
		ProjectFilePath =  FPaths::Combine(ProjectPath, ProjectName);
	}
	return ProjectFilePath;
}


bool UFlibPatchParserHelper::DiffVersionAssets(
	const FAssetDependenciesInfo& InNewVersion,
	const FAssetDependenciesInfo& InBaseVersion,
	FAssetDependenciesInfo& OutAddAsset,
	FAssetDependenciesInfo& OutModifyAsset,
	FAssetDependenciesInfo& OutDeleteAsset
)
{
	FAssetDependenciesInfo result;
	TArray<FString> AddAsset;
	TArray<FString> ModifyAsset;
	TArray<FString> DeleteAsset;

	{
		TArray<FString> InNewAssetModuleKeyList;
		InNewVersion.AssetsDependenciesMap.GetKeys(InNewAssetModuleKeyList);

		TArray<FString> InBaseAssetModuleKeysList;
		InBaseVersion.AssetsDependenciesMap.GetKeys(InBaseAssetModuleKeysList);

		// Parser Add new asset
		for (const auto& NewVersionAssetModule : InNewAssetModuleKeyList)
		{
			// is a new mdoule?
			if (!InBaseAssetModuleKeysList.Contains(NewVersionAssetModule))
			{
				OutAddAsset.AssetsDependenciesMap.Add(NewVersionAssetModule, *InNewVersion.AssetsDependenciesMap.Find(NewVersionAssetModule));
				continue;
			}
			{
				TArray<FString> NewVersionDependAssetsList;
				InNewVersion.AssetsDependenciesMap.Find(NewVersionAssetModule)->AssetDependencyDetails.GetKeys(NewVersionDependAssetsList);

				TArray<FString> BaseVersionDependAssetsList;
				InBaseVersion.AssetsDependenciesMap.Find(NewVersionAssetModule)->AssetDependencyDetails.GetKeys(BaseVersionDependAssetsList);

				const TMap<FString,FAssetDetail>& NewVersionAssetModuleDetail = InNewVersion.AssetsDependenciesMap.Find(NewVersionAssetModule)->AssetDependencyDetails;
				TArray<FString> CurrentModuleAssetList;
				NewVersionAssetModuleDetail.GetKeys(CurrentModuleAssetList);

				// add to TArray<FString>
				for (const auto& NewAssetItem : CurrentModuleAssetList)
				{
					if (!BaseVersionDependAssetsList.Contains(NewAssetItem))
					{
						FString BelongModuneName = UFlibAssetManageHelper::GetAssetBelongModuleName(NewAssetItem);
						if (!OutAddAsset.AssetsDependenciesMap.Contains(BelongModuneName))
						{
							OutAddAsset.AssetsDependenciesMap.Add(BelongModuneName, FAssetDependenciesDetail{ BelongModuneName,TMap<FString,FAssetDetail>{} });
						}
						TMap<FString, FAssetDetail>& CurrentModuleAssetDetails = OutAddAsset.AssetsDependenciesMap.Find(BelongModuneName)->AssetDependencyDetails;
						CurrentModuleAssetDetails.Add(NewAssetItem, *NewVersionAssetModuleDetail.Find(NewAssetItem));
					}
				}
			}	
		}

		// Parser Modify Asset
		for (const auto& BaseVersionAssetModule : InBaseAssetModuleKeysList)
		{
			const FAssetDependenciesDetail& BaseVersionModuleAssetsDetail = *InBaseVersion.AssetsDependenciesMap.Find(BaseVersionAssetModule);
			const FAssetDependenciesDetail& NewVersionModuleAssetsDetail = *InNewVersion.AssetsDependenciesMap.Find(BaseVersionAssetModule);


			if (InNewVersion.AssetsDependenciesMap.Contains(BaseVersionAssetModule))
			{
				TArray<FString> BeseVersionCurrentModuleAssetListKeys;
				BaseVersionModuleAssetsDetail.AssetDependencyDetails.GetKeys(BeseVersionCurrentModuleAssetListKeys);

				for (const auto& AssetItem : BeseVersionCurrentModuleAssetListKeys)
				{
					const FAssetDetail* BaseVersionAssetDetail = BaseVersionModuleAssetsDetail.AssetDependencyDetails.Find(AssetItem);
					const FAssetDetail* NewVersionAssetDetail = NewVersionModuleAssetsDetail.AssetDependencyDetails.Find(AssetItem);
					if (!NewVersionAssetDetail)
					{
						if (!OutDeleteAsset.AssetsDependenciesMap.Contains(BaseVersionAssetModule))
						{
							OutDeleteAsset.AssetsDependenciesMap.Add(BaseVersionAssetModule, FAssetDependenciesDetail{ BaseVersionAssetModule,TMap<FString,FAssetDetail>{} });
						}
						OutDeleteAsset.AssetsDependenciesMap.Find(BaseVersionAssetModule)->AssetDependencyDetails.Add(AssetItem, *BaseVersionAssetDetail);
						continue;
					}

					if (!(*NewVersionAssetDetail == *BaseVersionAssetDetail))
					{
						UE_LOG(LogHotPatcher,Display,TEXT("Modify Asset: %s"),*AssetItem);
						if (!OutModifyAsset.AssetsDependenciesMap.Contains(BaseVersionAssetModule))
						{
							OutModifyAsset.AssetsDependenciesMap.Add(BaseVersionAssetModule, FAssetDependenciesDetail{ BaseVersionAssetModule,TMap<FString,FAssetDetail>{} });
						}
						OutModifyAsset.AssetsDependenciesMap.Find(BaseVersionAssetModule)->AssetDependencyDetails.Add(AssetItem, *NewVersionAssetDetail);
					}
				}
			}
		}

	}

	return true;
}


bool UFlibPatchParserHelper::DiffVersionAllPlatformExFiles(
	const FExportPatchSettings& PatchSetting,
	const FHotPatcherVersion& InBaseVersion,const FHotPatcherVersion& InNewVersion, TMap<ETargetPlatform, FPatchVersionExternDiff>& OutDiff)
{
	OutDiff.Empty();
	auto ParserDiffPlatformExFileLambda = [](const FPlatformExternFiles& InBase,const FPlatformExternFiles& InNew,ETargetPlatform Platform)->FPatchVersionExternDiff
	{
		FPatchVersionExternDiff result;
		result.Platform = Platform;
		
		auto ParserAddFiles = [](const TArray<FExternFileInfo>& InBase,const TArray<FExternFileInfo>& InNew,TArray<FExternFileInfo>& Out)
		{
			for (const auto& NewVersionFile : InNew)
			{
				if (!InBase.Contains(NewVersionFile))
				{
					Out.AddUnique(NewVersionFile);
				}
			}
		};
	
		// Parser Add Files
		ParserAddFiles(InBase.ExternFiles, InNew.ExternFiles, result.AddExternalFiles);
		// Parser delete Files
		ParserAddFiles(InNew.ExternFiles, InBase.ExternFiles, result.DeleteExternalFiles);

		auto ParserModifyFiles = [](const FPlatformExternFiles& InBase,const FPlatformExternFiles& InNew,TArray<FExternFileInfo>& Out)
		{
			for (const auto& NewVersionFile : InNew.ExternFiles)
			{
				// UE_LOG(LogHotPatcher, Log, TEXT("check file %s."), *NewVersionFile);
				if (InBase.ExternFiles.Contains(NewVersionFile))
				{
					uint32 BaseFileIndex = InBase.ExternFiles.Find(NewVersionFile);
					bool bIsSame = (NewVersionFile.FileHash == InBase.ExternFiles[BaseFileIndex].FileHash);
					if (!bIsSame)
					{
						// UE_LOG(LogHotPatcher, Log, TEXT("%s is not same."), *NewVersionFile.MountPath);
						Out.Add(NewVersionFile);
					}else
					{
						// UE_LOG(LogHotPatcher, Log, TEXT("%s is same."), *NewVersionFile.MountPath);
					}
				}
				else
				{
					// UE_LOG(LogHotPatcher, Log, TEXT("base version not contains %s."), *NewVersionFile.MountPath);
				}
			}
		};
		
		// Parser modify Files
		ParserModifyFiles(InBase,InNew,result.ModifyExternalFiles);

		return result;
	};

	TArray<ETargetPlatform> NewPlatformKeys;
	TArray<ETargetPlatform> BasePlatformKeys;
	InNewVersion.PlatformAssets.GetKeys(NewPlatformKeys);
	InBaseVersion.PlatformAssets.GetKeys(BasePlatformKeys);

	// ADD
	for(const auto& Platform:NewPlatformKeys)
	{
		if(InBaseVersion.PlatformAssets.Contains(Platform))
		{
			OutDiff.Add(Platform,ParserDiffPlatformExFileLambda(
				UFlibPatchParserHelper::GetAllExFilesByPlatform(InBaseVersion.PlatformAssets[Platform],false,PatchSetting.GetHashCalculator()),
				UFlibPatchParserHelper::GetAllExFilesByPlatform(InNewVersion.PlatformAssets[Platform],true,PatchSetting.GetHashCalculator()),
				Platform
			));
		}
		else
		{
			FPlatformExternFiles EmptyBase;
			EmptyBase.Platform = Platform;
			OutDiff.Add(Platform,ParserDiffPlatformExFileLambda(
				EmptyBase,
                UFlibPatchParserHelper::GetAllExFilesByPlatform(InNewVersion.PlatformAssets[Platform],true,PatchSetting.GetHashCalculator()),
                Platform
            ));
		}
	}

	// Delete
	for(const auto& Platform:BasePlatformKeys)
	{
		if(!InNewVersion.PlatformAssets.Contains(Platform))
		{
			FPlatformExternFiles EmptyNew;
			EmptyNew.Platform = Platform;
			OutDiff.Add(Platform,ParserDiffPlatformExFileLambda(
                UFlibPatchParserHelper::GetAllExFilesByPlatform(InBaseVersion.PlatformAssets[Platform],false,PatchSetting.GetHashCalculator()),
                EmptyNew,
                Platform
            ));
		}
	}
	return true;
}

FPlatformExternFiles UFlibPatchParserHelper::GetAllExFilesByPlatform(
	const FPlatformExternAssets& InPlatformConf, bool InGeneratedHash, EHashCalculator HashCalculator)
{
	FPlatformExternFiles result;
	result.ExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(InPlatformConf.AddExternDirectoryToPak);

	for (auto& ExFile : InPlatformConf.AddExternFileToPak)
	{
		// ignore FPaths::FileExists(ExFile.FilePath.FilePath) 
		if (!ExFile.FilePath.FilePath.IsEmpty() && !result.ExternFiles.Contains(ExFile))
		{
			FExternFileInfo CurrentFile = ExFile;
			CurrentFile.FilePath.FilePath = UFlibPatchParserHelper::ReplaceMarkPath(ExFile.FilePath.FilePath);
			CurrentFile.MountPath = ExFile.MountPath;
			result.ExternFiles.Add(CurrentFile);
		}
	}
	if (InGeneratedHash)
	{
		for (auto& ExFile : result.ExternFiles)
		{
			ExFile.GenerateFileHash(HashCalculator);
		}
	}
	return result;
}

bool UFlibPatchParserHelper::GetPakFileInfo(const FString& InFile, FPakFileInfo& OutFileInfo)
{
	bool bRunStatus = false;
	if (FPaths::FileExists(InFile))
	{
		FString PathPart;
		FString FileNamePart;
		FString ExtensionPart;

		FPaths::Split(InFile, PathPart, FileNamePart, ExtensionPart);

		FMD5Hash CurrentPakHash = FMD5Hash::HashFile(*InFile);
		OutFileInfo.FileName = FString::Printf(TEXT("%s.%s"),*FileNamePart,*ExtensionPart);
		OutFileInfo.Hash = LexToString(CurrentPakHash);
		OutFileInfo.FileSize = IFileManager::Get().FileSize(*InFile);
		bRunStatus = true;
	}
	return bRunStatus;
}

TArray<FString> UFlibPatchParserHelper::GetCookedGlobalShaderCacheFiles(const FString& InProjectDir, const FString& InPlatformName)
{
	TArray<FString> Resault;
	// if (UFlibAssetManageHelper::IsValidPlatform(InPlatformName))
	{
		FString CookedEngineFolder = FPaths::Combine(InProjectDir,TEXT("Saved/Cooked"),InPlatformName,TEXT("Engine"));
		if (FPaths::DirectoryExists(CookedEngineFolder))
		{
			TArray<FString> FoundGlobalShaderCacheFiles;
			IFileManager::Get().FindFiles(FoundGlobalShaderCacheFiles, *CookedEngineFolder, TEXT("bin"));

			for (const auto& GlobalShaderCache : FoundGlobalShaderCacheFiles)
			{
				Resault.AddUnique(FPaths::Combine(CookedEngineFolder, GlobalShaderCache));
			}
		}
	}
	return Resault;
}


bool UFlibPatchParserHelper::GetCookedAssetRegistryFiles(const FString& InProjectAbsDir,const FString& InProjectName, const FString& InPlatformName, FString& OutFiles)
{
	bool bRunStatus = false;
	// if (UFlibAssetManageHelper::IsValidPlatform(InPlatformName))
	{
		FString CookedPAssetRegistryFile = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"), InPlatformName, InProjectName,TEXT("AssetRegistry.bin"));
		if (FPaths::FileExists(CookedPAssetRegistryFile))
		{
			bRunStatus = true;
			OutFiles = CookedPAssetRegistryFile;
		}
	}

	return bRunStatus;
}

bool UFlibPatchParserHelper::GetCookedShaderBytecodeFiles(const FString& InProjectAbsDir, const FString& InProjectName, const FString& InPlatformName, bool InGalobalBytecode, bool InProjectBytecode, TArray<FString>& OutFiles)
{
	bool bRunStatus = false;
	OutFiles.Reset();

	// if (UFlibAssetManageHelper::IsValidPlatform(InPlatformName))
	{
		FString CookedContentDir = FPaths::Combine(InProjectAbsDir, TEXT("Saved/Cooked"), InPlatformName, InProjectName, TEXT("Content"));

		if (FPaths::DirectoryExists(CookedContentDir))
		{
			TArray<FString> ShaderbytecodeFiles;
			IFileManager::Get().FindFiles(ShaderbytecodeFiles, *CookedContentDir, TEXT("ushaderbytecode"));

			for (const auto& ShaderByteCodeFile : ShaderbytecodeFiles)
			{
				if (InGalobalBytecode && ShaderByteCodeFile.Contains(TEXT("Global")))
				{
					OutFiles.AddUnique(FPaths::Combine(CookedContentDir, ShaderByteCodeFile));
				}
				if (InProjectBytecode && ShaderByteCodeFile.Contains(InProjectName))
				{
					OutFiles.AddUnique(FPaths::Combine(CookedContentDir, ShaderByteCodeFile));
				}
				
			}

			bRunStatus = !!ShaderbytecodeFiles.Num();
		}
	}
	return bRunStatus;
}

TArray<FString> UFlibPatchParserHelper::GetProjectIniFiles(const FString& InProjectDir, const FString& InPlatformName)
{
	FString ConfigFolder = FPaths::Combine(InProjectDir, TEXT("Config"));
	return UFlibPatchParserHelper::GetIniConfigs(ConfigFolder,InPlatformName);
	
}

bool UFlibPatchParserHelper::ConvIniFilesToPakCommands(
	const FString& InEngineAbsDir,
	const FString& InProjectAbsDir,
	const FString& InProjectName, 
	// const TArray<FString>& InPakOptions, 
	const TArray<FString>& InIniFiles, 
	TArray<FString>& OutCommands,
	TFunction<void(const FPakCommand&)> InReceiveCommand
)
{
	OutCommands.Reset();
	bool bRunStatus = false;
	if (!FPaths::DirectoryExists(InProjectAbsDir) || !FPaths::DirectoryExists(InEngineAbsDir))
		return false;
	FString UProjectFile = FPaths::Combine(InProjectAbsDir, InProjectName + TEXT(".uproject"));
	if (!FPaths::FileExists(UProjectFile))
		return false;

	for (const auto& IniFile : InIniFiles)
	{
		bool bIsInProjectIni = false;
		if (IniFile.Contains(InProjectAbsDir))
		{
			bIsInProjectIni = true;
		}

		{
			FString IniAbsDir;
			FString IniFileName;
			FString IniExtention;
			FPaths::Split(IniFile, IniAbsDir, IniFileName, IniExtention);

			FString RelativePath;

			if (bIsInProjectIni)
			{
				RelativePath = FString::Printf(
					TEXT("../../../%s/"), 
					*InProjectName
				);
				
				FString RelativeToProjectDir = UKismetStringLibrary::GetSubstring(IniAbsDir, InProjectAbsDir.Len(), IniAbsDir.Len() - InProjectAbsDir.Len());
				RelativePath = FPaths::Combine(RelativePath, RelativeToProjectDir);
			}
			else
			{
				RelativePath = FString::Printf(
					TEXT("../../../Engine/")
				);
				FString RelativeToEngineDir = UKismetStringLibrary::GetSubstring(IniAbsDir, InEngineAbsDir.Len(), IniAbsDir.Len() - InEngineAbsDir.Len());
				RelativePath = FPaths::Combine(RelativePath, RelativeToEngineDir);
			}

			FString IniFileNameWithExten = FString::Printf(TEXT("%s.%s"),*IniFileName,*IniExtention);
			FString CookedIniRelativePath = FPaths::Combine(RelativePath, IniFileNameWithExten);
			FString CookCommand = FString::Printf(
				TEXT("\"%s\" \"%s\""),
					*IniFile,
					*CookedIniRelativePath
			);
			FPakCommand CurrentPakCommand;
			CurrentPakCommand.PakCommands = TArray<FString>{ CookCommand };
			CurrentPakCommand.MountPath = CookedIniRelativePath;
			CurrentPakCommand.AssetPackage = UFlibPatchParserHelper::MountPathToRelativePath(CurrentPakCommand.MountPath);
			InReceiveCommand(CurrentPakCommand);
			OutCommands.AddUnique(CookCommand);
	
			bRunStatus = true;
		}
	}
	return bRunStatus;
}

bool UFlibPatchParserHelper::ConvNotAssetFileToPakCommand(
	const FString& InProjectDir, 
	const FString& InPlatformName,
	// const TArray<FString>& InPakOptions, 
	const FString& InCookedFile, 
	FString& OutCommand,
	TFunction<void(const FPakCommand&)> InReceiveCommand
)
{
	bool bRunStatus = false;
	if (FPaths::FileExists(InCookedFile))
	{
		FString CookPlatformAbsPath = FPaths::Combine(InProjectDir, TEXT("Saved/Cooked"), InPlatformName);

		FString RelativePath = UKismetStringLibrary::GetSubstring(InCookedFile, CookPlatformAbsPath.Len() + 1, InCookedFile.Len() - CookPlatformAbsPath.Len());
		FString AssetFileRelativeCookPath = FString::Printf(
			TEXT("../../../%s"),
			*RelativePath
		);

		OutCommand = FString::Printf(
			TEXT("\"%s\" \"%s\""),
			*InCookedFile,
			*AssetFileRelativeCookPath
		);
		FPakCommand CurrentPakCommand;
		CurrentPakCommand.PakCommands = TArray<FString>{ OutCommand };
		CurrentPakCommand.MountPath = AssetFileRelativeCookPath;
		CurrentPakCommand.AssetPackage = UFlibPatchParserHelper::MountPathToRelativePath(CurrentPakCommand.MountPath);
		InReceiveCommand(CurrentPakCommand);
		bRunStatus = true;
	}
	return bRunStatus;
}

// bool UFlibPatchParserHelper::ConvNotAssetFileToExFile(const FString& InProjectDir, const FString& InPlatformName, const FString& InCookedFile, FExternFileInfo& OutExFile)
// {
// 	bool bRunStatus = false;
// 	if (FPaths::FileExists(InCookedFile))
// 	{
// 		FString CookPlatformAbsPath = FPaths::Combine(InProjectDir, TEXT("Saved/Cooked"), InPlatformName);
//
// 		FString RelativePath = UKismetStringLibrary::GetSubstring(InCookedFile, CookPlatformAbsPath.Len() + 1, InCookedFile.Len() - CookPlatformAbsPath.Len());
// 		FString AssetFileRelativeCookPath = FString::Printf(
// 			TEXT("../../../%s"),
// 			*RelativePath
// 		);
//
// 		OutExFile.FilePath.FilePath = InCookedFile;
// 		OutExFile.MountPath = AssetFileRelativeCookPath;
// 		OutExFile.GetFileHash();
// 		bRunStatus = true;
// 	}
// 	return bRunStatus;
// }

FString UFlibPatchParserHelper::HashStringWithSHA1(const FString &InString)
{
	FSHAHash StringHash;
	FSHA1::HashBuffer(TCHAR_TO_ANSI(*InString), InString.Len(), StringHash.Hash);
	return StringHash.ToString();

}


TArray<FString> UFlibPatchParserHelper::GetIniConfigs(const FString& InSearchDir, const FString& InPlatformName)
{
	TArray<FString> Result;
	const FString SearchConfigAbsDir = FPaths::ConvertRelativePathToFull(InSearchDir);

	if (FPaths::DirectoryExists(SearchConfigAbsDir))
	{
		FFileArrayDirectoryVisitor Visitor;

		IFileManager::Get().IterateDirectory(*SearchConfigAbsDir, Visitor);

		for (const auto& IniFile : Visitor.Files)
		{
			if (!FPaths::GetCleanFilename(IniFile).Contains(TEXT("Editor")))
			{
				Result.Add(IniFile);
			}
		}
		int32 PlatformNameBeginIndex = SearchConfigAbsDir.Len() + 1;
		for (const auto& PlatformIniDirectory : Visitor.Directories)
		{
			FString DirectoryName = UKismetStringLibrary::GetSubstring(PlatformIniDirectory, PlatformNameBeginIndex, PlatformIniDirectory.Len() - PlatformNameBeginIndex);
			if (InPlatformName.Contains(DirectoryName))
			{
				FFileArrayDirectoryVisitor PlatformVisitor;

				IFileManager::Get().IterateDirectory(*PlatformIniDirectory, PlatformVisitor);

				for (const auto& PlatformIni : PlatformVisitor.Files)
				{
					Result.Add(PlatformIni);
				}
			}
		}
	}
	return Result;
}

TArray<FString> UFlibPatchParserHelper::GetEngineConfigs(const FString& InPlatformName)
{
	return UFlibPatchParserHelper::GetIniConfigs(FPaths::EngineConfigDir() , InPlatformName);
}

TArray<FString> UFlibPatchParserHelper::GetEnabledPluginConfigs(const FString& InPlatformName)
{
	TArray<FString> result;
	TArray<TSharedRef<IPlugin>> AllEnablePlugins = IPluginManager::Get().GetEnabledPlugins();

	for (const auto& Plugin : AllEnablePlugins)
	{
		FString PluginAbsPath;
		if (UFlibAssetManageHelper::GetPluginModuleAbsDir(Plugin->GetName(), PluginAbsPath))
		{
			FString PluginIniPath = FPaths::Combine(PluginAbsPath, TEXT("Config"));
			result.Append(UFlibPatchParserHelper::GetIniConfigs(PluginIniPath, InPlatformName));
			
		}
	}

	return result;
}


TArray<FExternFileInfo> UFlibPatchParserHelper::ParserExDirectoryAsExFiles(const TArray<FExternDirectoryInfo>& InExternDirectorys)
{
	TArray<FExternFileInfo> result;

	if (!InExternDirectorys.Num())
		return result;

	for (const auto& DirectoryItem : InExternDirectorys)
	{
		if(DirectoryItem.DirectoryPath.Path.IsEmpty())
			continue;
		FString DirAbsPath = UFlibPatchParserHelper::ReplaceMarkPath(DirectoryItem.DirectoryPath.Path); //FPaths::ConvertRelativePathToFull(DirectoryItem.DirectoryPath.Path);
		bool bWildcard = DirectoryItem.bWildcard;
		FString Wildcard = DirectoryItem.WildcardStr;
		FPaths::MakeStandardFilename(DirAbsPath);
		if (!DirAbsPath.IsEmpty() && FPaths::DirectoryExists(DirAbsPath))
		{
			TArray<FString> DirectoryAllFiles;
			if (UFlibAssetManageHelper::FindFilesRecursive(DirAbsPath, DirectoryAllFiles, true))
			{
				int32 ParentDirectoryIndex = DirAbsPath.Len();
				for (const auto& File : DirectoryAllFiles)
				{
					FString RelativeParentPath = UKismetStringLibrary::GetSubstring(File, ParentDirectoryIndex, File.Len() - ParentDirectoryIndex);
					FString RelativeMountPointPath = FPaths::Combine(DirectoryItem.MountPoint, RelativeParentPath);
					FPaths::MakeStandardFilename(RelativeMountPointPath);

					FExternFileInfo CurrentFile;
					CurrentFile.FilePath.FilePath = File;

					CurrentFile.MountPath = RelativeMountPointPath;
					bool bCanAdd = true;
					if(bWildcard)
					{
						bCanAdd = File.MatchesWildcard(Wildcard) && RelativeMountPointPath.MatchesWildcard(Wildcard);
					}
					if (!result.Contains(CurrentFile) && bCanAdd)
					{
						result.Add(CurrentFile);
					}
				}
			}
		}
	}
	return result;
}


TArray<FAssetDetail> UFlibPatchParserHelper::ParserExFilesInfoAsAssetDetailInfo(const TArray<FExternFileInfo>& InExFiles)
{
	TArray<FAssetDetail> result;

	for (auto& File : InExFiles)
	{
		FAssetDetail CurrentFile;
		CurrentFile.AssetType = TEXT("ExternalFile");
		CurrentFile.PackagePath = FName(*File.MountPath);
		//CurrentFile.mGuid = File.GetFileHash();
		result.Add(CurrentFile);
	}

	return result;

}


TArray<FString> UFlibPatchParserHelper::GetIniFilesByPakInternalInfo(const FPakInternalInfo& InPakInternalInfo, const FString& PlatformName)
{
	TArray<FString> Inis;
	if (InPakInternalInfo.bIncludeEngineIni)
	{
		Inis.Append(UFlibPatchParserHelper::GetEngineConfigs(PlatformName));
	}
	if (InPakInternalInfo.bIncludePluginIni)
	{
		Inis.Append(UFlibPatchParserHelper::GetEnabledPluginConfigs(PlatformName));
	}
	if (InPakInternalInfo.bIncludeProjectIni)
	{
		Inis.Append(UFlibPatchParserHelper::GetProjectIniFiles(FPaths::ProjectDir(), PlatformName));
	}

	return Inis;
}


TArray<FString> UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(const FPakInternalInfo& InPakInternalInfo, const FString& PlatformName)
{
	TArray<FString> SearchAssetList;

	FString ProjectAbsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString ProjectName = UFlibPatchParserHelper::GetProjectName();

	if (InPakInternalInfo.bIncludeAssetRegistry)
	{
		FString AssetRegistryCookCommand;
		if (UFlibPatchParserHelper::GetCookedAssetRegistryFiles(ProjectAbsDir, UFlibPatchParserHelper::GetProjectName(), PlatformName, AssetRegistryCookCommand))
		{
			SearchAssetList.AddUnique(AssetRegistryCookCommand);
		}
	}

	if (InPakInternalInfo.bIncludeGlobalShaderCache)
	{
		TArray<FString> GlobalShaderCacheList = UFlibPatchParserHelper::GetCookedGlobalShaderCacheFiles(ProjectAbsDir, PlatformName);
		if (!!GlobalShaderCacheList.Num())
		{
			SearchAssetList.Append(GlobalShaderCacheList);
		}
	}

	if (InPakInternalInfo.bIncludeShaderBytecode)
	{
		TArray<FString> ShaderByteCodeFiles;

		if (UFlibPatchParserHelper::GetCookedShaderBytecodeFiles(ProjectAbsDir, ProjectName, PlatformName, true, true, ShaderByteCodeFiles) &&
			!!ShaderByteCodeFiles.Num()
			)
		{
			SearchAssetList.Append(ShaderByteCodeFiles);
		}
	}
	return SearchAssetList;
}


// TArray<FExternFileInfo> UFlibPatchParserHelper::GetInternalFilesAsExFiles(const FPakInternalInfo& InPakInternalInfo, const FString& InPlatformName)
// {
// 	TArray<FExternFileInfo> resultFiles;
//
// 	TArray<FString> AllNeedPakInternalCookedFiles;
//
// 	FString ProjectAbsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
// 	AllNeedPakInternalCookedFiles.Append(UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(InPakInternalInfo, InPlatformName));
//
// 	for (const auto& File : AllNeedPakInternalCookedFiles)
// 	{
// 		FExternFileInfo CurrentFile;
// 		UFlibPatchParserHelper::ConvNotAssetFileToExFile(ProjectAbsDir,InPlatformName, File, CurrentFile);
// 		resultFiles.Add(CurrentFile);
// 	}
//
// 	return resultFiles;
// }

TArray<FString> UFlibPatchParserHelper::GetPakCommandsFromInternalInfo(
	const FPakInternalInfo& InPakInternalInfo, 
	const FString& PlatformName, 
	// const TArray<FString>& InPakOptions, 
	TFunction<void(const FPakCommand&)> InReceiveCommand
)
{
	TArray<FString> OutPakCommands;
	TArray<FString> AllNeedPakInternalCookedFiles;

	FString ProjectAbsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	AllNeedPakInternalCookedFiles.Append(UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(InPakInternalInfo, PlatformName));

	// combine as cook commands
	for (const auto& AssetFile : AllNeedPakInternalCookedFiles)
	{
		FString CurrentCommand;
		if (UFlibPatchParserHelper::ConvNotAssetFileToPakCommand(ProjectAbsDir, PlatformName ,/*InPakOptions,*/ AssetFile, CurrentCommand, InReceiveCommand))
		{
			OutPakCommands.AddUnique(CurrentCommand);
		}
	}

	FString EngineAbsDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());

	auto CombineInPakCommands = [&OutPakCommands, &ProjectAbsDir, &EngineAbsDir, &PlatformName, /*&InPakOptions,*/&InReceiveCommand](const TArray<FString>& IniFiles)
	{
		TArray<FString> result;

		TArray<FString> IniPakCommmands;
		UFlibPatchParserHelper::ConvIniFilesToPakCommands(
			EngineAbsDir,
			ProjectAbsDir,
			UFlibPatchParserHelper::GetProjectName(),
			// InPakOptions,
			IniFiles,
			IniPakCommmands,
			InReceiveCommand
		);
		if (!!IniPakCommmands.Num())
		{
			OutPakCommands.Append(IniPakCommmands);
		}
	};

	TArray<FString> AllNeedPakIniFiles = UFlibPatchParserHelper::GetIniFilesByPakInternalInfo(InPakInternalInfo, PlatformName);
	CombineInPakCommands(AllNeedPakIniFiles);

	return OutPakCommands;
}


FChunkInfo UFlibPatchParserHelper::CombineChunkInfo(const FChunkInfo& R, const FChunkInfo& L)
{
	FChunkInfo result = R;

	result.ChunkName = FString::Printf(TEXT("%s_%s"), *R.ChunkName, *L.ChunkName);
	result.AssetIncludeFilters.Append(L.AssetIncludeFilters);
	result.AssetIgnoreFilters.Append(L.AssetIgnoreFilters);
	result.bAnalysisFilterDependencies = L.bAnalysisFilterDependencies || R.bAnalysisFilterDependencies;
	result.IncludeSpecifyAssets.Append(L.IncludeSpecifyAssets);
	// result.AddExternDirectoryToPak.Append(L.AddExternDirectoryToPak);
	// result.AddExternFileToPak.Append(L.AddExternFileToPak);
	
	for(const auto& LChunkPlatform:L.AddExternAssetsToPlatform)
	{
		int32 FoundIndex = result.AddExternAssetsToPlatform.Find(LChunkPlatform);
		if(FoundIndex!=INDEX_NONE)
		{
			result.AddExternAssetsToPlatform[FoundIndex].AddExternDirectoryToPak.Append(LChunkPlatform.AddExternDirectoryToPak);
			result.AddExternAssetsToPlatform[FoundIndex].AddExternFileToPak.Append(LChunkPlatform.AddExternFileToPak);
		}
		else
		{
			result.AddExternAssetsToPlatform.Add(LChunkPlatform);
		}
	}
#define COMBINE_INTERNAL_FILES(InResult,Left,Right,PropertyName)\
	InResult.InternalFiles.PropertyName = Left.InternalFiles.PropertyName || Right.InternalFiles.PropertyName;

	COMBINE_INTERNAL_FILES(result, L, R, bIncludeAssetRegistry);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeGlobalShaderCache);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeShaderBytecode);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeEngineIni);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludePluginIni);
	COMBINE_INTERNAL_FILES(result, L, R, bIncludeProjectIni);

	return result;
}

FChunkInfo UFlibPatchParserHelper::CombineChunkInfos(const TArray<FChunkInfo>& Chunks)
{
	FChunkInfo result;
	
	for (const auto& Chunk : Chunks)
	{
		result = UFlibPatchParserHelper::CombineChunkInfo(result, Chunk);
	}
	return result;
}

TArray<FString> UFlibPatchParserHelper::GetDirectoryPaths(const TArray<FDirectoryPath>& InDirectoryPath)
{
	TArray<FString> Result;
	for (const auto& Filter : InDirectoryPath)
	{
		if (!Filter.Path.IsEmpty())
		{
			FString Path = Filter.Path;
			if(Path.StartsWith(TEXT("/All/")))
			{
				Path.RemoveFromStart(TEXT("/All"));
			}
			Result.AddUnique(Path);
		}
	}
	return Result;
}

TMap<ETargetPlatform,FPlatformExternFiles> UFlibPatchParserHelper::GetAllPlatformExternFilesFromChunk(const FChunkInfo& InChunk, bool bCalcHash)
{
	TMap<ETargetPlatform,FPlatformExternFiles> result;
	auto ParserAllFileAndDirs = [](const FPlatformExternAssets& InPlatformInfo,bool bCalcHash=true)->FPlatformExternFiles
	{
		FPlatformExternFiles result;
		result.Platform = InPlatformInfo.TargetPlatform;
		result.ExternFiles = InPlatformInfo.AddExternFileToPak;
		for (auto& ExFile : InPlatformInfo.AddExternFileToPak)
		{
			if (!result.ExternFiles.Contains(ExFile))
			{
				result.ExternFiles.Add(ExFile);
			}
		}
		if (bCalcHash)
		{
			for (auto& ExFile : result.ExternFiles)
			{
				ExFile.GenerateFileHash();
			}
		}
		return result;
	};
	
	for(const auto& Platform:InChunk.AddExternAssetsToPlatform)
	{
		result.Add(Platform.TargetPlatform,ParserAllFileAndDirs(Platform));
	}
	
	return result;
}

FChunkAssetDescribe UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(
	const FHotPatcherSettingBase* PatcheSettings,
	const FPatchVersionDiff& DiffInfo,
	const FChunkInfo& Chunk, TArray<ETargetPlatform> Platforms
)
{
	SCOPED_NAMED_EVENT_TEXT("CollectFChunkAssetsDescribeByChunk",FColor::Red);
	FChunkAssetDescribe ChunkAssetDescribe;
	// Collect Chunk Assets
	{
		
		FAssetDependenciesInfo SpecifyDependAssets;
		
		FAssetDependenciesParser Parser;
		FAssetDependencies Conf;
		TArray<FString> AssetFilterPaths = UFlibAssetManageHelper::DirectoriesToStrings(Chunk.AssetIncludeFilters);
		Conf.IncludeFilters = AssetFilterPaths;
		Conf.IgnoreFilters = UFlibAssetManageHelper::DirectoriesToStrings(Chunk.AssetIgnoreFilters);
		Conf.ForceSkipPackageNames = UFlibAssetManageHelper::SoftObjectPathsToStrings(Chunk.ForceSkipAssets);
		Conf.InIncludeSpecifyAsset = Chunk.IncludeSpecifyAssets;
		Conf.AssetRegistryDependencyTypes = Chunk.AssetRegistryDependencyTypes;
		Conf.AnalysicFilterDependencies = Chunk.bAnalysisFilterDependencies;
		Conf.ForceSkipContents = UFlibAssetManageHelper::DirectoriesToStrings(Chunk.ForceSkipContentRules);
		Conf.ForceSkipContents.Append(UFlibAssetManageHelper::DirectoriesToStrings(PatcheSettings->GetAssetScanConfig().ForceSkipContentRules));
		
		auto AddForceSkipAssets = [&Conf](const TArray<FSoftObjectPath>& Assets)
		{
			for(const auto& forceSkipAsset:Assets)
			{
				Conf.ForceSkipContents.Add(forceSkipAsset.GetLongPackageName());
			}
		};
		AddForceSkipAssets(Chunk.ForceSkipAssets);
		AddForceSkipAssets(PatcheSettings->GetAssetScanConfig().ForceSkipAssets);
		
		auto AddSkipClassesLambda = [&Conf](const TArray<UClass*>& Classes)
		{
			for(const auto& ForceSkipClass:Classes)
			{
				Conf.IgnoreAseetTypes.Add(*ForceSkipClass->GetName());
			}
		};
		AddSkipClassesLambda(Chunk.ForceSkipClasses);
		AddSkipClassesLambda(PatcheSettings->GetAssetScanConfig().ForceSkipClasses);
		
		Parser.Parse(Conf);
		TSet<FName> AssetLongPackageNames = Parser.GetrParseResults();
	
		for(FName LongPackageName:AssetLongPackageNames)
		{
			if(LongPackageName.IsNone())
			{
				continue;
			}
			AssetFilterPaths.AddUnique(LongPackageName.ToString());
		}
	
		const FAssetDependenciesInfo& AddAssetsRef = DiffInfo.AssetDiffInfo.AddAssetDependInfo;
		const FAssetDependenciesInfo& ModifyAssetsRef = DiffInfo.AssetDiffInfo.ModifyAssetDependInfo;
	
	
		auto CollectChunkAssets = [](const FAssetDependenciesInfo& SearchBase, const TArray<FString>& SearchFilters)->FAssetDependenciesInfo
		{
			SCOPED_NAMED_EVENT_TEXT("CollectChunkAssets",FColor::Red);
			FAssetDependenciesInfo ResultAssetDependInfos;
	
			for (const auto& SearchItem : SearchFilters)
			{
				if (SearchItem.IsEmpty())
					continue;
	
				FString SearchModuleName;
				int32 findedPos = SearchItem.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1);
				if (findedPos != INDEX_NONE)
				{
					SearchModuleName = UKismetStringLibrary::GetSubstring(SearchItem, 1, findedPos - 1);
				}
				else
				{
					SearchModuleName = UKismetStringLibrary::GetSubstring(SearchItem, 1, SearchItem.Len() - 1);
				}
	
				if (!SearchModuleName.IsEmpty() && (SearchBase.AssetsDependenciesMap.Contains(SearchModuleName)))
				{
					if (!ResultAssetDependInfos.AssetsDependenciesMap.Contains(SearchModuleName))
						ResultAssetDependInfos.AssetsDependenciesMap.Add(SearchModuleName, FAssetDependenciesDetail(SearchModuleName, TMap<FString, FAssetDetail>{}));
	
					const FAssetDependenciesDetail& SearchBaseModule = *SearchBase.AssetsDependenciesMap.Find(SearchModuleName);
	
					TArray<FString> AllAssetKeys;
					SearchBaseModule.AssetDependencyDetails.GetKeys(AllAssetKeys);
	
					for (const auto& KeyItem : AllAssetKeys)
					{
						if (KeyItem.StartsWith(SearchItem))
						{
							FAssetDetail FindedAsset = *SearchBaseModule.AssetDependencyDetails.Find(KeyItem);
							if (!ResultAssetDependInfos.AssetsDependenciesMap.Find(SearchModuleName)->AssetDependencyDetails.Contains(KeyItem))
							{
								ResultAssetDependInfos.AssetsDependenciesMap.Find(SearchModuleName)->AssetDependencyDetails.Add(KeyItem, FindedAsset);
							}
						}
					}
				}
			}
			return ResultAssetDependInfos;
		};

		ChunkAssetDescribe.AddAssets = CollectChunkAssets(AddAssetsRef, AssetFilterPaths);
		ChunkAssetDescribe.ModifyAssets = CollectChunkAssets(ModifyAssetsRef, AssetFilterPaths);
		ChunkAssetDescribe.Assets = UFlibAssetManageHelper::CombineAssetDependencies(ChunkAssetDescribe.AddAssets, ChunkAssetDescribe.ModifyAssets);
	}

	auto CoolectPlatformExFiles = [](const FPatchVersionDiff& DiffInfo,const auto& Chunk,ETargetPlatform Platform)->TArray<FExternFileInfo>
	{
		SCOPED_NAMED_EVENT_TEXT("CoolectPlatformExFiles",FColor::Red);
		// Collect Extern Files
		TArray<FExternFileInfo> AllFiles;

		TSet<FString> AllSearchFileFilter;
		{
			for(int32 PlatformIndex=0;PlatformIndex<Chunk.AddExternAssetsToPlatform.Num();++PlatformIndex)
			{
				if(Chunk.AddExternAssetsToPlatform[PlatformIndex].TargetPlatform == Platform)
				{
					for (const auto& Directory : Chunk.AddExternAssetsToPlatform[PlatformIndex].AddExternDirectoryToPak)
					{
						if (!Directory.DirectoryPath.Path.IsEmpty())
							AllSearchFileFilter.Add(UFlibPatchParserHelper::ReplaceMarkPath(Directory.DirectoryPath.Path));
					}
					for (const auto& File : Chunk.AddExternAssetsToPlatform[PlatformIndex].AddExternFileToPak)
					{
						AllSearchFileFilter.Add(UFlibPatchParserHelper::ReplaceMarkPath(File.FilePath.FilePath));
					}
				}
			}
		}
		TArray<FExternFileInfo> AddFilesRef;
		TArray<FExternFileInfo> ModifyFilesRef;
		if(DiffInfo.PlatformExternDiffInfo.Contains(Platform))
		{
			AddFilesRef = DiffInfo.PlatformExternDiffInfo.Find(Platform)->AddExternalFiles;
			ModifyFilesRef = DiffInfo.PlatformExternDiffInfo.Find(Platform)->ModifyExternalFiles;
		}

		// TArray<FString>
		auto CollectExtenFilesLambda = [&AllFiles](const TArray<FExternFileInfo>& SearchBase, const TSet<FString>& Filters,EPatchAssetType Type)
		{
			for (const auto& Filter : Filters)
			{
				for (const auto& SearchItem : SearchBase)
				{
					if (SearchItem.FilePath.FilePath.StartsWith(Filter))
					{
						FExternFileInfo FileItem = SearchItem;
						FileItem.Type = Type;
						AllFiles.Add(FileItem);
					}
				}
			}
		};
		CollectExtenFilesLambda(AddFilesRef, AllSearchFileFilter,EPatchAssetType::NEW);
		CollectExtenFilesLambda(ModifyFilesRef, AllSearchFileFilter,EPatchAssetType::MODIFY);
		return AllFiles;
	};

	for(auto Platform:Platforms)
	{
		FPlatformExternFiles PlatformFiles;
		PlatformFiles.Platform = Platform;
		PlatformFiles.ExternFiles = CoolectPlatformExFiles(DiffInfo,Chunk,Platform);
		ChunkAssetDescribe.AllPlatformExFiles.Add(Platform,PlatformFiles);
	}
	
	ChunkAssetDescribe.InternalFiles = Chunk.InternalFiles;
	return ChunkAssetDescribe;
}


TArray<FString> UFlibPatchParserHelper::CollectPakCommandsStringsByChunk(
	const FPatchVersionDiff& DiffInfo,
	const FChunkInfo& Chunk,
	const FString& PlatformName,
	// const TArray<FString>& PakOptions,
	const FExportPatchSettings* PatcheSettings
)
{
	TArray<FString> ChunkPakCommands;
	{
		TArray<FPakCommand> ChunkPakCommands_r = UFlibPatchParserHelper::CollectPakCommandByChunk(DiffInfo, Chunk, PlatformName,/* PakOptions,*/PatcheSettings);
		for (const auto& PakCommand : ChunkPakCommands_r)
		{
			ChunkPakCommands.Append(PakCommand.GetPakCommands());
		}
	}

	return ChunkPakCommands;
}

TArray<FPakCommand> UFlibPatchParserHelper::CollectPakCommandByChunk(
	const FPatchVersionDiff& DiffInfo,
	const FChunkInfo& Chunk,
	const FString& PlatformName,
	// const TArray<FString>& PakOptions,
	const FExportPatchSettings* PatcheSettings
)
{
	TArray<FPakCommand> PakCommands;
	auto CollectPakCommandsByChunkLambda = [&PakCommands,PatcheSettings](const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName/*, const TArray<FString>& PakOptions*/)
	{
		ETargetPlatform Platform;
		THotPatcherTemplateHelper::GetEnumValueByName(PlatformName,Platform);
		TArray<ETargetPlatform> CollectPlatforms = {ETargetPlatform::AllPlatforms};
		CollectPlatforms.AddUnique(Platform);
		FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(PatcheSettings, DiffInfo ,Chunk, CollectPlatforms);
		
		bool bIoStore =false;
		bool bAllowBulkDataInIoStore = false;
#if ENGINE_MAJOR_VERSION > 4 ||ENGINE_MINOR_VERSION > 25
		if(PatcheSettings)
		{
			bIoStore = PatcheSettings->GetIoStoreSettings().bIoStore;
			bAllowBulkDataInIoStore = PatcheSettings->GetIoStoreSettings().bAllowBulkDataInIoStore;
		}
#endif
		
		auto IsIoStoreAssetsLambda = [bIoStore,bAllowBulkDataInIoStore](const FString& InAbsAssets)->bool
		{
			bool Matched = false;
			if(bIoStore)
			{
				TArray<FString> IoStoreExterns = {TEXT(".uasset"),TEXT(".umap")};
				if(bAllowBulkDataInIoStore)
				{
					IoStoreExterns.Add(TEXT(".ubulk"));
					IoStoreExterns.Add(TEXT(".uptnl"));
				}
				// bool bIoStoreMatched = false;
				for(const auto& IoStoreExrern:IoStoreExterns)
				{
					if(InAbsAssets.EndsWith(IoStoreExrern))
					{
						Matched = true;
						break;
					}
				}
			}
			return Matched;
		};
		

		// Collect Chunk Assets
		{
			struct PakCommandReceiver
			{
				PakCommandReceiver(TArray<FPakCommand>& InPakCommandsRef,EPatchAssetType InType):PakCommands(InPakCommandsRef),Type(InType){}
				void operator()(const TArray<FString>& InPakCommand,const TArray<FString>& InIoStoreCommand, const FString& InMountPath,const FString& InLongPackageName)
				{
					FPakCommand CurrentPakCommand;
					CurrentPakCommand.PakCommands = InPakCommand;
					CurrentPakCommand.IoStoreCommands = InIoStoreCommand;
					CurrentPakCommand.MountPath = InMountPath;
					CurrentPakCommand.AssetPackage = InLongPackageName;
					CurrentPakCommand.Type = Type;
					PakCommands.Add(CurrentPakCommand);
				}
				TArray<FPakCommand>& PakCommands;
				EPatchAssetType Type;
			};
			// auto ReceivePakCommandAssetLambda = [&PakCommands](const TArray<FString>& InPakCommand,const TArray<FString>& InIoStoreCommand, const FString& InMountPath,const FString& InLongPackageName)
			// {
			// 	
			// };
			PakCommandReceiver AddReceiver(PakCommands,EPatchAssetType::NEW);
			PakCommandReceiver ModifyReceiver(PakCommands,EPatchAssetType::MODIFY);
			FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
			TArray<FString> AssetsPakCommands;
			UFlibAssetManageHelper::MakePakCommandFromAssetDependencies(
				ProjectDir,
				PatcheSettings->GetStorageCookedDir(),
				PlatformName,
				ChunkAssetsDescrible.AddAssets,
				// PakOptions,
				AssetsPakCommands,
				AddReceiver,
				IsIoStoreAssetsLambda
			);
			AssetsPakCommands.Empty();
			UFlibAssetManageHelper::MakePakCommandFromAssetDependencies(
				ProjectDir,
				PatcheSettings->GetStorageCookedDir(),
				PlatformName,
				ChunkAssetsDescrible.ModifyAssets,
				// PakOptions,
				AssetsPakCommands,
				ModifyReceiver,
				IsIoStoreAssetsLambda
			);
		}
		
		// Collect Extern Files
		for(const auto& PlatformItenm:CollectPlatforms)
		{
			for (const auto& CollectFile : ChunkAssetsDescrible.AllPlatformExFiles[PlatformItenm].ExternFiles)
			{
				FPakCommand CurrentPakCommand;
				CurrentPakCommand.MountPath = CollectFile.MountPath;
				CurrentPakCommand.AssetPackage = UFlibPatchParserHelper::MountPathToRelativePath(CurrentPakCommand.MountPath);
				
				// FString PakOptionsStr;
				// for (const auto& Param : PakOptions)
				// {
				// 	FString FileExtension = FPaths::GetExtension(CollectFile.FilePath.FilePath,false);
				// 	if(IgnoreCompressFormats.Contains(FileExtension) && Param.Equals(TEXT("-compress"),ESearchCase::IgnoreCase))
				// 	{
				// 		continue;
				// 	}
				// 	PakOptionsStr += TEXT(" ") + Param;
				// }
				
				CurrentPakCommand.PakCommands = TArray<FString>{ FString::Printf(TEXT("\"%s\" \"%s\""), *CollectFile.FilePath.FilePath, *CollectFile.MountPath/*,*PakOptionsStr*/) };
				CurrentPakCommand.Type = CollectFile.Type;
				PakCommands.Add(CurrentPakCommand);
			}
		}
		// internal files
		// TArray<FString> NotCompressOptions = PakOptions;
		// if(NotCompressOptions.Contains(TEXT("-compress")))
		// 	NotCompressOptions.Remove(TEXT("-compress"));
		auto ReceivePakCommandExFilesLambda = [&PakCommands](const FPakCommand& InCommand){ PakCommands.Emplace(InCommand); };
		UFlibPatchParserHelper::GetPakCommandsFromInternalInfo(ChunkAssetsDescrible.InternalFiles, PlatformName,/* NotCompressOptions,*/ ReceivePakCommandExFilesLambda);
	};
	CollectPakCommandsByChunkLambda(DiffInfo, Chunk, PlatformName/*, PakOptions*/);
	return PakCommands;
}

FHotPatcherVersion UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
	const FString& InVersionId,
	const FString& InBaseVersion,
	const FString& InDate,
	const FChunkInfo& InChunkInfo,
	bool InIncludeHasRefAssetsOnly /*= false */,
	bool bInAnalysisFilterDependencies /* = true*/
	, EHashCalculator HashCalculator)
{
	TArray<FString> AllSkipContents;;
	if(InChunkInfo.bForceSkipContent)
	{
		AllSkipContents.Append(UFlibAssetManageHelper::DirectoriesToStrings(InChunkInfo.ForceSkipContentRules));
		AllSkipContents.Append(UFlibAssetManageHelper::SoftObjectPathsToStrings(InChunkInfo.ForceSkipAssets));
	}

	SCOPED_NAMED_EVENT_TEXT("ExportReleaseVersionInfo",FColor::Red);
	FHotPatcherVersion ExportVersion;
	{
		ExportVersion.VersionId = InVersionId;
		ExportVersion.Date = InDate;
		ExportVersion.BaseVersionId = InBaseVersion;
	}

	FAssetScanConfig ScanConfig;
	ScanConfig.bPackageTracker = false;
	ScanConfig.AssetIncludeFilters = InChunkInfo.AssetIncludeFilters;
	ScanConfig.AssetIgnoreFilters = InChunkInfo.AssetIgnoreFilters;
	ScanConfig.bAnalysisFilterDependencies = bInAnalysisFilterDependencies;
	ScanConfig.bRecursiveWidgetTree = false;
	ScanConfig.IncludeSpecifyAssets = InChunkInfo.IncludeSpecifyAssets;
	ScanConfig.bForceSkipContent = InChunkInfo.bForceSkipContent;
	ScanConfig.ForceSkipAssets = InChunkInfo.ForceSkipAssets;
	ScanConfig.ForceSkipClasses = InChunkInfo.ForceSkipClasses;
	ScanConfig.ForceSkipContentRules = InChunkInfo.ForceSkipContentRules;
	ScanConfig.AssetRegistryDependencyTypes = InChunkInfo.AssetRegistryDependencyTypes;
	ScanConfig.bIncludeHasRefAssetsOnly = InIncludeHasRefAssetsOnly;
	RunAssetScanner(ScanConfig,ExportVersion);

	bool CalcHash = HashCalculator == EHashCalculator::NoHash ? false : true;
	ExportExternAssetsToPlatform(InChunkInfo.AddExternAssetsToPlatform,ExportVersion,CalcHash,HashCalculator);

	return ExportVersion;
}

void UFlibPatchParserHelper::RunAssetScanner(FAssetScanConfig ScanConfig,FHotPatcherVersion& ExportVersion)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibPatchParserHelper::RunAssetScanner",FColor::Red);

	TSharedPtr<FPackageTrackerBase> ObjectTracker;
	if(ScanConfig.bPackageTracker)
	{
		ObjectTracker = MakeShareable(new FPackageTrackerBase);
	}
	// add Include Filter
	{
		TArray<FString> AllIncludeFilter;
		for (const auto& Filter : ScanConfig.AssetIncludeFilters)
		{
			AllIncludeFilter.AddUnique(Filter.Path);
			ExportVersion.IncludeFilter.AddUnique(Filter.Path);
		}
	}
	// add Ignore Filter
	{
		TArray<FString> AllIgnoreFilter;
		for (const auto& Filter : ScanConfig.AssetIgnoreFilters)
		{
			AllIgnoreFilter.AddUnique(Filter.Path);
			ExportVersion.IgnoreFilter.AddUnique(Filter.Path);
		}
	}
	ExportVersion.bIncludeHasRefAssetsOnly = ScanConfig.bIncludeHasRefAssetsOnly;
	ExportVersion.IncludeSpecifyAssets = ScanConfig.IncludeSpecifyAssets;

	TSet<FName> IgnoreTypes = 
	{
		TEXT("EditorUtilityBlueprint")
	};
	
	FAssetDependencies AssetConfig;
	AssetConfig.IncludeFilters = UFlibAssetManageHelper::NormalizeContentDirs(ExportVersion.IncludeFilter);
	AssetConfig.IgnoreFilters = UFlibAssetManageHelper::NormalizeContentDirs(ExportVersion.IgnoreFilter);
	
	AssetConfig.AssetRegistryDependencyTypes = ScanConfig.AssetRegistryDependencyTypes;
	AssetConfig.InIncludeSpecifyAsset = ScanConfig.IncludeSpecifyAssets;

	
	if(ScanConfig.bForceSkipContent)
	{
		TArray<FString> AllSkipContents;;
		AllSkipContents.Append(UFlibAssetManageHelper::DirectoriesToStrings(ScanConfig.ForceSkipContentRules));
		for(auto Class:ScanConfig.ForceSkipClasses)
		{
			IgnoreTypes.Add(*Class->GetName());
		}
		AssetConfig.ForceSkipContents = UFlibAssetManageHelper::NormalizeContentDirs(AllSkipContents);
		AssetConfig.ForceSkipPackageNames = UFlibAssetManageHelper::SoftObjectPathsToStrings(ScanConfig.ForceSkipAssets);
	}
	
	AssetConfig.bRedirector = true;
	AssetConfig.AnalysicFilterDependencies = ScanConfig.bAnalysisFilterDependencies;
	AssetConfig.IncludeHasRefAssetsOnly = ScanConfig.bIncludeHasRefAssetsOnly;
	AssetConfig.IgnoreAseetTypes = IgnoreTypes;
	AssetConfig.bSupportWorldComposition = ScanConfig.bSupportWorldComposition;
	{
		SCOPED_NAMED_EVENT_TEXT("parser all uasset dependencies(optimized)",FColor::Red);
		FAssetDependenciesParser Parser;
		Parser.Parse(AssetConfig);

		UE_LOG(LogHotPatcher,Display,TEXT("FAssetDependenciteParser Asset Num (Optimized) %d"),Parser.GetrParseResults().Num());

		{
			SCOPED_NAMED_EVENT_TEXT("combine all assets to FAssetDependenciesInfo",FColor::Red);
			FCriticalSection	SynchronizationObject;
			const TArray<FName>& ParseResultSet = Parser.GetrParseResults().Array();
			ParallelFor(Parser.GetrParseResults().Num(),[&](int32 index)
			{
				FName LongPackageName = ParseResultSet[index];
				if(LongPackageName.IsNone())
				{
					return;
				}
				FAssetDetail CurrentDetail;
				if(UFlibAssetManageHelper::GetSpecifyAssetDetail(LongPackageName.ToString(),CurrentDetail))
				{
					UFlibAssetManageHelper::IsRedirector(CurrentDetail,CurrentDetail);
					{
						FScopeLock Lock(&SynchronizationObject);
						ExportVersion.AssetInfo.AddAssetsDetail(CurrentDetail);
					}
				}
			},GForceSingleThread);
		}
	}
	
	if(ObjectTracker && ScanConfig.bPackageTracker)
	{
		SCOPED_NAMED_EVENT_TEXT("Combine Package Tracker Assets",FColor::Red);
		const TArray<FName> LoadedPackages = ObjectTracker->GetLoadedPackages().Array();
		
		FCriticalSection	SynchronizationObject;
		ParallelFor(LoadedPackages.Num(),[&](int32 index)
		{
			FAssetDetail LoadedPackageDetail = UFlibAssetManageHelper::GetAssetDetailByPackageName(LoadedPackages[index].ToString());
			UFlibAssetManageHelper::IsRedirector(LoadedPackageDetail,LoadedPackageDetail);
			
			{
				FScopeLock Lock(&SynchronizationObject);
				ExportVersion.AssetInfo.AddAssetsDetail(LoadedPackageDetail);
			}
		},GForceSingleThread);
	}
}

void UFlibPatchParserHelper::ExportExternAssetsToPlatform(
	const TArray<FPlatformExternAssets>& AddExternAssetsToPlatform, FHotPatcherVersion& ExportVersion, bool
	bGenerateHASH, EHashCalculator HashCalculator)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibPatchParserHelper::ExportExternAssetsToPlatform",FColor::Red);
	for(const auto& PlatformExInfo:AddExternAssetsToPlatform)
	{
		FPlatformExternFiles PlatformFileInfo = UFlibPatchParserHelper::GetAllExFilesByPlatform(PlatformExInfo,bGenerateHASH,HashCalculator);
		FPlatformExternAssets PlatformExternAssets;
		PlatformExternAssets.TargetPlatform = PlatformExInfo.TargetPlatform;
		PlatformExternAssets.AddExternFileToPak = PlatformFileInfo.ExternFiles;
		ExportVersion.PlatformAssets.Add(PlatformExInfo.TargetPlatform,PlatformExternAssets);
	}
}

TArray<FString> UFlibPatchParserHelper::GetPakCommandStrByCommands(const TArray<FPakCommand>& PakCommands,const TArray<FReplaceText>& InReplaceTexts,bool bIoStore)
{
	SCOPED_NAMED_EVENT_TEXT("UFlibPatchParserHelper::ExportExternAssetsToPlatform",FColor::Red);
	TArray<FString> ResultPakCommands;
	{
		FCriticalSection	SynchronizationObject;
		ParallelFor(PakCommands.Num(),[&](int32 index)
		{
			const TArray<FString>& PakCommandOriginTexts = bIoStore?PakCommands[index].GetIoStoreCommands():PakCommands[index].GetPakCommands();
			if (!!InReplaceTexts.Num())
			{
				for (const auto& PakCommandOriginText : PakCommandOriginTexts)
				{
					FString PakCommandTargetText = PakCommandOriginText;
					for (const auto& ReplaceText : InReplaceTexts)
					{
						ESearchCase::Type SearchCaseMode = ReplaceText.SearchCase == ESearchCaseMode::CaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
						PakCommandTargetText = PakCommandTargetText.Replace(*ReplaceText.From, *ReplaceText.To, SearchCaseMode);
					}
					{
						FScopeLock Lock(&SynchronizationObject);
						ResultPakCommands.Add(PakCommandTargetText);
					}
				}
			}
			else
			{
				FScopeLock Lock(&SynchronizationObject);
				ResultPakCommands.Append(PakCommandOriginTexts);
			}
		},GForceSingleThread);
	}
	return ResultPakCommands;
}



// FHotPatcherAssetDependency UFlibPatchParserHelper::GetAssetRelatedInfo(
// 	const FAssetDetail& InAsset,
// 	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
// 	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
// )
// {
// 	FHotPatcherAssetDependency Dependency;
// 	Dependency.Asset = InAsset;
// 	FString LongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(InAsset.PackagePath.ToString());
// 	
// 	{
// 		TArray<EAssetRegistryDependencyType::Type> SearchAssetDepTypes;
// 		for (const auto& Type : AssetRegistryDependencyTypes)
// 		{
// 			SearchAssetDepTypes.AddUnique(UFlibAssetManageHelper::ConvAssetRegistryDependencyToInternal(Type));
// 		}
//
// 		UFlibAssetManageHelper::GetAssetDependency(LongPackageName, AssetRegistryDependencyTypes,Dependency.AssetDependency, ScanedCaches,false);
// 		UFlibAssetManageHelper::GetAssetReference(InAsset, SearchAssetDepTypes, Dependency.AssetReference);
//
// 	}
//
// 	return Dependency;
// }
//
// TArray<FHotPatcherAssetDependency> UFlibPatchParserHelper::GetAssetsRelatedInfo(
// 	const TArray<FAssetDetail>& InAssets,
// 	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
// 	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
// )
// {
// 	TArray<FHotPatcherAssetDependency> AssetsDependency;
//
// 	for (const auto& Asset : InAssets)
// 	{
// 		AssetsDependency.Add(UFlibPatchParserHelper::GetAssetRelatedInfo(Asset, AssetRegistryDependencyTypes,ScanedCaches));
// 	}
// 	return AssetsDependency;
// }
//
// TArray<FHotPatcherAssetDependency> UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(
// 	const FAssetDependenciesInfo& InAssetsDependencies,
// 	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
// 	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
// )
// {
// 	const TArray<FAssetDetail>& AssetsDetail = InAssetsDependencies.GetAssetDetails();
// 	return UFlibPatchParserHelper::GetAssetsRelatedInfo(AssetsDetail, AssetRegistryDependencyTypes,ScanedCaches);
// }



bool UFlibPatchParserHelper::GetCookProcCommandParams(const FCookerConfig& InConfig, FString& OutParams)
{
	FString FinalParams;
	FinalParams.Append(FString::Printf(TEXT("\"%s\" "), *InConfig.ProjectPath));
	FinalParams.Append(FString::Printf(TEXT("%s "), *InConfig.EngineParams));

	auto CombineParamsLambda = [&FinalParams,&InConfig](const FString& InName, const TArray<FString>& InArray)
	{
		if(!InArray.Num())
			return;
		FinalParams.Append(InName);

		for (int32 Index = 0; Index < InArray.Num(); ++Index)
		{
			FinalParams.Append(InArray[Index]);
			if (!(Index == InArray.Num() - 1))
			{
				FinalParams.Append(TEXT("+"));
			}
		}
		FinalParams.Append(TEXT(" "));
	};
	CombineParamsLambda(TEXT("-targetplatform="), InConfig.CookPlatforms);
	CombineParamsLambda(TEXT("-Map="), InConfig.CookMaps);
	
	for (const auto& CookFilter : InConfig.CookFilter)
	{
		FString FilterFullPath;
		if (UFlibAssetManageHelper::ConvRelativeDirToAbsDir(CookFilter, FilterFullPath))
		{
			if (FPaths::DirectoryExists(FilterFullPath))
			{
				FinalParams.Append(FString::Printf(TEXT("-COOKDIR=\"%s\" "), *FilterFullPath));
			}
		}
	}

	for (const auto& Option : InConfig.CookSettings)
	{
		FinalParams.Append(TEXT("-") + Option + TEXT(" "));
	}

	FinalParams.Append(InConfig.Options);

	OutParams = FinalParams;

	return true;
}

void UFlibPatchParserHelper::ExcludeContentForVersionDiff(FPatchVersionDiff& VersionDiff,const TArray<FString>& ExcludeRules,EHotPatcherMatchModEx matchMod)
{
	UFlibAssetManageHelper::ExcludeContentForAssetDependenciesDetail(VersionDiff.AssetDiffInfo.AddAssetDependInfo,ExcludeRules,matchMod);
	UFlibAssetManageHelper::ExcludeContentForAssetDependenciesDetail(VersionDiff.AssetDiffInfo.ModifyAssetDependInfo,ExcludeRules,matchMod);
	UFlibAssetManageHelper::ExcludeContentForAssetDependenciesDetail(VersionDiff.AssetDiffInfo.DeleteAssetDependInfo,ExcludeRules,matchMod);
}

FString UFlibPatchParserHelper::MountPathToRelativePath(const FString& InMountPath)
{
	FString Path;
	FString filename;
	FString RelativePath;
#define RELATIVE_STR_LENGTH 9
	if (InMountPath.StartsWith("../../../"))
	{
		RelativePath = UKismetStringLibrary::GetSubstring(InMountPath, RELATIVE_STR_LENGTH, InMountPath.Len() - RELATIVE_STR_LENGTH);
	}
	FString extersion;
	FPaths::Split(RelativePath, Path, filename, extersion);
	return Path/filename;
}

#define ENGINEDIR_MARK TEXT("[ENGINEDIR]/")
#define ENGINE_CONTENT_DIR_MARK TEXT("[ENGINE_CONTENT_DIR]/")
#define PROJECTDIR_MARK TEXT("[PROJECTDIR]/")
#define PROJECT_CONTENT_DIR_MARK TEXT("[PROJECT_CONTENT_DIR]/")
#define PROJECT_SAVED_DIR_MARK TEXT("[PROJECT_SAVED_DIR]/")
#define PROJECT_CONFIG_DIR_MARK TEXT("[PROJECT_CONFIG_DIR]/")

TMap<FString, FString> UFlibPatchParserHelper::GetReplacePathMarkMap()
{
	static TMap<FString,FString> MarkMap;
	static bool bInited = false;
	if(!bInited)
	{
		MarkMap.Add(ENGINEDIR_MARK,FPaths::EngineDir());
		MarkMap.Add(ENGINE_CONTENT_DIR_MARK,FPaths::EngineContentDir());
		MarkMap.Add(PROJECTDIR_MARK,FPaths::ProjectDir());
		MarkMap.Add(PROJECT_CONTENT_DIR_MARK,FPaths::ProjectContentDir());
		MarkMap.Add(PROJECT_SAVED_DIR_MARK,FPaths::ProjectSavedDir());
		MarkMap.Add(PROJECT_CONFIG_DIR_MARK,FPaths::ProjectConfigDir());
		bInited = true;
	}
	
	return MarkMap;
}

FString UFlibPatchParserHelper::ReplaceMark(const FString& Src)
{
	TMap<FString,FString> MarkMap = UFlibPatchParserHelper::GetReplacePathMarkMap();
	TArray<FString> MarkKeys;
	MarkMap.GetKeys(MarkKeys);
		
	FString result = Src;
	for(const auto& Key:MarkKeys)
	{
		while(result.Contains(Key))
		{
			result = result.Replace(*Key,*FPaths::ConvertRelativePathToFull(*MarkMap.Find(Key)));
		}
	}
	FPaths::NormalizeFilename(result);
	FPaths::CollapseRelativeDirectories(result);
	return result;
}

FString UFlibPatchParserHelper::ReplaceMarkPath(const FString& Src)
{
	FString result = UFlibPatchParserHelper::ReplaceMark(Src);
	FPaths::MakeStandardFilename(result);
	result = FPaths::ConvertRelativePathToFull(result);
	return result;
}

void UFlibPatchParserHelper::ReplacePatherSettingProjectDir(TArray<FPlatformExternAssets>& PlatformAssets)
{
	for(auto& ExPlatform:PlatformAssets)
	{
		for(auto& ExFile:ExPlatform.AddExternFileToPak)
		{
			ExFile.FilePath.FilePath = UFlibPatchParserHelper::ReplaceMarkPath(ExFile.FilePath.FilePath);
		}
		for(auto& ExDir:ExPlatform.AddExternDirectoryToPak)
		{
			ExDir.DirectoryPath.Path = UFlibPatchParserHelper::ReplaceMarkPath(ExDir.DirectoryPath.Path);
		}
	}
}

TArray<FString> UFlibPatchParserHelper::GetUnCookUassetExtensions()
{
	TArray<FString> UAssetFormats = { TEXT(".uasset"),TEXT(".umap")};
	return UAssetFormats;
}

TArray<FString> UFlibPatchParserHelper::GetCookedUassetExtensions()
{
	TArray<FString> UAssetFormats = { TEXT(".uasset"),TEXT(".ubulk"),TEXT(".uexp"),TEXT(".umap"),TEXT(".ufont") };
	return UAssetFormats;
}

bool UFlibPatchParserHelper::IsCookedUassetExtensions(const FString& InAsset)
{
	return UFlibPatchParserHelper::MatchStrInArray(InAsset,UFlibPatchParserHelper::GetCookedUassetExtensions());
}
bool UFlibPatchParserHelper::IsUnCookUassetExtension(const FString& InAsset)
{
	return UFlibPatchParserHelper::MatchStrInArray(InAsset,UFlibPatchParserHelper::GetUnCookUassetExtensions());
}

bool UFlibPatchParserHelper::MatchStrInArray(const FString& InStr, const TArray<FString>& InArray)
{
	bool bResult = false;
	for (const auto& Format:InArray)
	{
		if (InStr.Contains(Format))
		{
			bResult = true;
			break;
		}
	}
	return bResult;
}

FString UFlibPatchParserHelper::LoadAESKeyStringFromCryptoFile(const FString& InCryptoJson)
{
	FString result;
	if(FPaths::FileExists(InCryptoJson))
	{
		FString Content;
		FFileHelper::LoadFileToString(Content,*InCryptoJson);
		if (!Content.IsEmpty())
		{
			TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);
			TSharedPtr<FJsonObject> DeserializeJsonObject;
			if (FJsonSerializer::Deserialize(JsonReader, DeserializeJsonObject))
			{
				const TSharedPtr<FJsonObject>* EncryptionKeyObject;
				DeserializeJsonObject->TryGetObjectField(TEXT("EncryptionKey"),EncryptionKeyObject);
				if(EncryptionKeyObject->IsValid())
				{
					FString Key;
					EncryptionKeyObject->Get()->TryGetStringField(TEXT("Key"),Key);
					if(!Key.IsEmpty())
					{
						result = Key;
					}
				}
			}
		}
	}
	return result;
}

FAES::FAESKey UFlibPatchParserHelper::LoadAESKeyFromCryptoFile(const FString& InCryptoJson)
{
	FAES::FAESKey AESKey;
	FString AESKeyString = UFlibPatchParserHelper::LoadAESKeyStringFromCryptoFile(InCryptoJson);
	if(!AESKeyString.IsEmpty())
	{
		TArray<uint8> DecodedBuffer;
		if (!FBase64::Decode(AESKeyString, DecodedBuffer))
		{
			FMemory::Memcpy(AESKey.Key, DecodedBuffer.GetData(), 32);
		}
	}
	return AESKey;
}

FString UFlibPatchParserHelper::AssetMountPathToAbs(const FString& InAssetMountPath)
{
	FString result;
	// ../../../Engine/xxx to ENGINE/
	FString Dest = InAssetMountPath;
	while(Dest.StartsWith(TEXT("../")))
	{
		Dest.RemoveFromStart(TEXT("../"));
	}

	if(Dest.StartsWith(TEXT("Engine")))
	{
		Dest.RemoveFromStart(TEXT("Engine/"));
		result = FPaths::Combine(FPaths::EngineDir(),Dest);	
	}
	if(Dest.StartsWith(FApp::GetProjectName()))
	{
		Dest.RemoveFromStart(FString::Printf(TEXT("%s/"),FApp::GetProjectName()));
		result = FPaths::Combine(FPaths::ProjectDir(),Dest);	
	}
	FPaths::NormalizeFilename(result);
	return FPaths::ConvertRelativePathToFull(result);
}

FString UFlibPatchParserHelper::UAssetMountPathToPackagePath(const FString& InAssetMountPath)
{
	TMap<FString, FString> ReceiveModuleMap;
	UFlibAssetManageHelper::GetAllEnabledModuleName(ReceiveModuleMap);

	FString LongPackageName = InAssetMountPath;

	LongPackageName.RemoveFromStart(TEXT("../../.."));

	for(const auto& UnCookAssetExtension:UFlibPatchParserHelper::GetUnCookUassetExtensions())
	{
		LongPackageName.RemoveFromEnd(UnCookAssetExtension);
	}
	
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

	FString LongPackagePath = UFlibAssetManageHelper::LongPackageNameToPackagePath(LongPackageName);
	return LongPackagePath;
}

bool UFlibPatchParserHelper::GetPluginPakPathByName(const FString& PluginName,FString& uPluginAbsPath,FString& uPluginMountPath)
{
	bool bStatus = false;
	TSharedPtr<IPlugin> FoundPlugin = IPluginManager::Get().FindPlugin(PluginName);
	FString uPluginFileAbsPath;
	FString uPluginFileMountPath;
	if(FoundPlugin.IsValid())
	{
		uPluginFileAbsPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FoundPlugin->GetBaseDir(),FString::Printf(TEXT("%s.uplugin"),*FoundPlugin->GetName())));
		FPaths::NormalizeFilename(uPluginFileAbsPath);
				
		uPluginFileMountPath = uPluginFileAbsPath;
				
		if(FPaths::FileExists(uPluginFileAbsPath))
		{
			FString EnginePluginPath = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir());
			FString ProjectPluginPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());
			if(uPluginFileMountPath.StartsWith(EnginePluginPath))
			{
				uPluginFileMountPath.RemoveFromStart(EnginePluginPath);
				uPluginFileMountPath = FString::Printf(TEXT("../../../Engine/Plugins/%s"),*uPluginFileMountPath);
			}
			if(uPluginFileMountPath.StartsWith(ProjectPluginPath))
			{
				uPluginFileMountPath.RemoveFromStart(ProjectPluginPath);
				uPluginFileMountPath = FString::Printf(TEXT("../../../%s/Plugins/%s"),FApp::GetProjectName(),*uPluginFileMountPath);
			}
			uPluginAbsPath = uPluginFileAbsPath;
			uPluginMountPath = uPluginFileMountPath;
			bStatus = true;
		}
	}
	return bStatus;
}

FString UFlibPatchParserHelper::GetPluginMountPoint(const FString& PluginName)
{
	FString uPluginAbsPath;
	FString uPluginMountPath;
	if(UFlibPatchParserHelper::GetPluginPakPathByName(PluginName,uPluginAbsPath,uPluginMountPath))
	{
		uPluginMountPath.RemoveFromEnd(FString::Printf(TEXT("/%s.uplugin"),*PluginName));
	}
	return uPluginMountPath;
}

FString UFlibPatchParserHelper::ParserMountPointRegular(const FString& Src)
{
	FString result;
	if(Src.MatchesWildcard(FString::Printf(TEXT("*%s*"),AS_PROJECTDIR_MARK)))
	{
		result = Src.Replace(AS_PROJECTDIR_MARK,*FString::Printf(TEXT("../../../%s"),FApp::GetProjectName()));
	}
	if(Src.MatchesWildcard(FString::Printf(TEXT("*%s*"),AS_PLUGINDIR_MARK)))
	{
		int32 index = Src.Find(AS_PLUGINDIR_MARK);
		index += strlen(TCHAR_TO_ANSI(AS_PLUGINDIR_MARK));
		int32 BeginIndex = index + 1; 
		int32 EndIndex;
		Src.FindLastChar(']',EndIndex);
		FString PluginName = UKismetStringLibrary::GetSubstring(Src,BeginIndex,EndIndex-BeginIndex);
		result = Src.Replace(AS_PLUGINDIR_MARK,TEXT(""));
		result = result.Replace(*FString::Printf(TEXT("[%s]"),*PluginName),*UFlibPatchParserHelper::GetPluginMountPoint(PluginName));
	}
	return result;
}

void UFlibPatchParserHelper::ReloadShaderbytecode()
{
	UFlibPakHelper::ReloadShaderbytecode();
}

bool UFlibPatchParserHelper::LoadShaderbytecode(const FString& LibraryName, const FString& LibraryDir)
{
	return UFlibPakHelper::LoadShaderbytecode(LibraryName,LibraryDir);
}

void UFlibPatchParserHelper::CloseShaderbytecode(const FString& LibraryName)
{
	UFlibPakHelper::CloseShaderbytecode(LibraryName);
}

#define ENCRYPT_KEY_NAME TEXT("EncryptionKey")
#define ENCRYPT_PAK_INI_FILES_NAME TEXT("bEncryptPakIniFiles")
#define ENCRYPT_PAK_INDEX_NAME TEXT("bEncryptPakIndex")
#define ENCRYPT_UASSET_FILES_NAME TEXT("bEncryptUAssetFiles")
#define ENCRYPT_ALL_ASSET_FILES_NAME TEXT("bEncryptAllAssetFiles")

#define SIGNING_PAK_SIGNING_NAME TEXT("bEnablePakSigning")
#define SIGNING_MODULES_NAME TEXT("SigningModulus")
#define SIGNING_PUBLIC_EXPONENT_NAME TEXT("SigningPublicExponent")
#define SIGNING_PRIVATE_EXPONENT_NAME TEXT("SigningPrivateExponent")

FPakEncryptionKeys UFlibPatchParserHelper::GetCryptoByProjectSettings()
{
	FPakEncryptionKeys result;

	result.EncryptionKey.Name = TEXT("Embedded");
	result.EncryptionKey.Guid = FGuid::NewGuid().ToString();
	
	UClass* Class = FindObject<UClass>(ANY_PACKAGE, TEXT("/Script/CryptoKeys.CryptoKeysSettings"), true);
	if(Class)
	{
		FString AESKey;
		for(TFieldIterator<FProperty> PropertyIter(Class);PropertyIter;++PropertyIter)
		{
			FProperty* PropertyIns = *PropertyIter;
			// UE_LOG(LogHotPatcher,Log,TEXT("%s"),*PropertyIns->GetName());
			if(PropertyIns->GetName().Equals(ENCRYPT_KEY_NAME))
			{
				result.EncryptionKey.Key = *PropertyIns->ContainerPtrToValuePtr<FString>(Class->GetDefaultObject());
			}
			if(PropertyIns->GetName().Equals(ENCRYPT_PAK_INI_FILES_NAME))
			{
				result.bEnablePakIniEncryption = *PropertyIns->ContainerPtrToValuePtr<bool>(Class->GetDefaultObject());
			}
			if(PropertyIns->GetName().Equals(ENCRYPT_PAK_INDEX_NAME))
			{
				result.bEnablePakIndexEncryption = *PropertyIns->ContainerPtrToValuePtr<bool>(Class->GetDefaultObject());
			}
			if(PropertyIns->GetName().Equals(ENCRYPT_UASSET_FILES_NAME))
			{
				result.bEnablePakUAssetEncryption = *PropertyIns->ContainerPtrToValuePtr<bool>(Class->GetDefaultObject());
			}
			if(PropertyIns->GetName().Equals(ENCRYPT_ALL_ASSET_FILES_NAME))
			{
				result.bEnablePakFullAssetEncryption = *PropertyIns->ContainerPtrToValuePtr<bool>(Class->GetDefaultObject());
			}
			// SIGN
			if(PropertyIns->GetName().Equals(SIGNING_PAK_SIGNING_NAME))
			{
				result.bEnablePakSigning = *PropertyIns->ContainerPtrToValuePtr<bool>(Class->GetDefaultObject());
			}
			if(PropertyIns->GetName().Equals(SIGNING_PUBLIC_EXPONENT_NAME))
			{
				result.SigningKey.PublicKey.Exponent = *PropertyIns->ContainerPtrToValuePtr<FString>(Class->GetDefaultObject());
			}
			if(PropertyIns->GetName().Equals(SIGNING_MODULES_NAME))
			{
				result.SigningKey.PublicKey.Modulus = *PropertyIns->ContainerPtrToValuePtr<FString>(Class->GetDefaultObject());
				result.SigningKey.PrivateKey.Modulus = result.SigningKey.PublicKey.Modulus;
			}
			if(PropertyIns->GetName().Equals(SIGNING_PRIVATE_EXPONENT_NAME))
			{
				result.SigningKey.PrivateKey.Exponent = *PropertyIns->ContainerPtrToValuePtr<FString>(Class->GetDefaultObject());
			}
		}
	}
	return result;
}

FEncryptSetting UFlibPatchParserHelper::GetCryptoSettingsByJson(const FString& CryptoJson)
{
	FEncryptSetting result;
	FArchive* File = IFileManager::Get().CreateFileReader(*CryptoJson);
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<char>> Reader = TJsonReaderFactory<char>::Create(File);
	if (FJsonSerializer::Deserialize(Reader, RootObject))
	{
		result.bSign = RootObject->GetBoolField(TEXT("bEnablePakSigning"));
		result.bEncryptIndex = RootObject->GetBoolField(TEXT("bEnablePakIndexEncryption"));
		result.bEncryptIniFiles = RootObject->GetBoolField(TEXT("bEnablePakIniEncryption"));
		result.bEncryptUAssetFiles = RootObject->GetBoolField(TEXT("bEnablePakUAssetEncryption"));
		result.bEncryptAllAssetFiles = RootObject->GetBoolField(TEXT("bEnablePakFullAssetEncryption"));
	}
	return result;
}

FEncryptSetting UFlibPatchParserHelper::GetCryptoSettingByPakEncryptSettings(const FPakEncryptSettings& Config)
{
	FEncryptSetting EncryptSettings;
	if(Config.bUseDefaultCryptoIni)
	{
		FPakEncryptionKeys ProjectCrypt = UFlibPatchParserHelper::GetCryptoByProjectSettings();
		EncryptSettings.bEncryptIniFiles = ProjectCrypt.bEnablePakIniEncryption;
		EncryptSettings.bEncryptUAssetFiles = ProjectCrypt.bEnablePakUAssetEncryption;
		EncryptSettings.bEncryptAllAssetFiles = ProjectCrypt.bEnablePakFullAssetEncryption;
		EncryptSettings.bEncryptIndex = ProjectCrypt.bEnablePakIndexEncryption;
		EncryptSettings.bSign = ProjectCrypt.bEnablePakSigning;
	}
	else
	{
		FString CryptoKeyFile = UFlibPatchParserHelper::ReplaceMark(Config.CryptoKeys.FilePath);
		if(FPaths::FileExists(CryptoKeyFile))
		{
			FEncryptSetting CryptoJsonSettings = UFlibPatchParserHelper::GetCryptoSettingsByJson(CryptoKeyFile);
			EncryptSettings.bEncryptIniFiles = CryptoJsonSettings.bEncryptIniFiles;
			EncryptSettings.bEncryptUAssetFiles = CryptoJsonSettings.bEncryptUAssetFiles;
			EncryptSettings.bEncryptAllAssetFiles = CryptoJsonSettings.bEncryptAllAssetFiles;
			EncryptSettings.bSign = CryptoJsonSettings.bSign;
			EncryptSettings.bEncryptIndex = CryptoJsonSettings.bEncryptIndex;
		}
	}
	return EncryptSettings;
}

bool UFlibPatchParserHelper::SerializePakEncryptionKeyToFile(const FPakEncryptionKeys& PakEncryptionKeys,
                                                                  const FString& ToFile)
{
	FString KeyInfo;
	THotPatcherTemplateHelper::TSerializeStructAsJsonString(PakEncryptionKeys,KeyInfo);
	return UFlibAssetManageHelper::SaveStringToFile(ToFile, KeyInfo);
}

TArray<FDirectoryPath> UFlibPatchParserHelper::GetDefaultForceSkipContentDir()
{
	TArray<FDirectoryPath> result;
	TArray<FString> DefaultSkipEditorContentRules = {
		TEXT("/Engine/Editor*/")
		,TEXT("/Engine/VREditor/")
	};

	TArray<FString> AlwaySkipTempContentRules = {
#if ENGINE_MAJOR_VERSION
		TEXT("/Game/__ExternalActors__")
		,TEXT("/Game/__ExternalObjects__")
#endif
	};
	
	auto AddToSkipDirs = [&result](const TArray<FString>& SkipDirs)
	{
		for(const auto& Ruls:SkipDirs)
		{
			FDirectoryPath PathIns;
			PathIns.Path = Ruls;
			result.Add(PathIns);
		}
	};
	AddToSkipDirs(AlwaySkipTempContentRules);

	bool bSkipEditorContent = false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"),TEXT("bSkipEditorContent"),bSkipEditorContent,GGameIni);

	if(bSkipEditorContent)
	{
		AddToSkipDirs(DefaultSkipEditorContentRules);
	}
	
	return result;
}

FSHAHash UFlibPatchParserHelper::FileSHA1Hash(const FString& Filename)
{
	FSHAHash Hash;
	FSHA1::GetFileSHAHash(*Filename,Hash.Hash,true);
	TArray<uint8> Data;
	FFileHelper::LoadFileToArray(Data,*Filename);
	FSHA1::HashBuffer(Data.GetData(),Data.Num(),Hash.Hash);
	return Hash;
}

FString UFlibPatchParserHelper::FileHash(const FString& Filename, EHashCalculator Calculator)
{
	FString HashValue;
	
	FString FileAbsPath = FPaths::ConvertRelativePathToFull(Filename);
	if (FPaths::FileExists(FileAbsPath))
	{
		switch (Calculator)
		{
		case EHashCalculator::MD5:
			{
				FMD5Hash FileMD5Hash = FMD5Hash::HashFile(*FileAbsPath);
				HashValue = LexToString(FileMD5Hash);
				break;
			}
		case  EHashCalculator::SHA1:
			{
				FSHAHash Hash = UFlibPatchParserHelper::FileSHA1Hash(Filename);
				HashValue = Hash.ToString();
				break;
			}
		};
	}
	return HashValue;
}

bool UFlibPatchParserHelper::IsValidPatchSettings(const FExportPatchSettings* PatchSettings,bool bExternalFilesCheck)
{
	bool bCanExport = false;
	FExportPatchSettings* NoConstSettingObject = const_cast<FExportPatchSettings*>(PatchSettings);
	if (NoConstSettingObject)
	{
		bool bHasBase = false;
		if (NoConstSettingObject->IsByBaseVersion())
			bHasBase = !NoConstSettingObject->GetBaseVersion().IsEmpty() && FPaths::FileExists(NoConstSettingObject->GetBaseVersion());
		else
			bHasBase = true;
		bool bHasVersionId = !NoConstSettingObject->GetVersionId().IsEmpty();
		bool bHasFilter = !!NoConstSettingObject->GetAssetIncludeFilters().Num();
		bool bHasSpecifyAssets = !!NoConstSettingObject->GetIncludeSpecifyAssets().Num();
		bool bHasSavePath = !NoConstSettingObject->GetSaveAbsPath().IsEmpty();
		bool bHasPakPlatfotm = !!NoConstSettingObject->GetPakTargetPlatforms().Num();

		bool bHasExternFiles = true;
		if(bExternalFilesCheck)
		{
			bHasExternFiles = !!NoConstSettingObject->GetAllPlatfotmExternFiles().Num();
		}
		
		bool bHasExDirs = !!NoConstSettingObject->GetAddExternAssetsToPlatform().Num();
		
		bool bHasAnyPakFiles = (
			bHasFilter || bHasSpecifyAssets || bHasExternFiles || bHasExDirs ||
			NoConstSettingObject->IsIncludeEngineIni() ||
			NoConstSettingObject->IsIncludePluginIni() ||
			NoConstSettingObject->IsIncludeProjectIni() ||
			NoConstSettingObject->bImportProjectSettings
			);
		bCanExport = bHasBase && bHasVersionId && bHasAnyPakFiles && bHasPakPlatfotm && bHasSavePath;
	}
	return bCanExport;
}

FString UFlibPatchParserHelper::GetTargetPlatformsStr(const TArray<ETargetPlatform>& Platforms)
{
	FString PlatformArrayStr;
	TArray<ETargetPlatform> UniquePlatforms;
	for(ETargetPlatform Platform:Platforms){ UniquePlatforms.AddUnique(Platform);}
	for(ETargetPlatform Platform:UniquePlatforms)
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		PlatformArrayStr += FString::Printf(TEXT("%s+"),*PlatformName);
	}
	PlatformArrayStr.RemoveFromEnd(TEXT("+"));
	return PlatformArrayStr;
}

FString UFlibPatchParserHelper::GetTargetPlatformsCmdLine(const TArray<ETargetPlatform>& Platforms)
{
	FString Result;
	if(Platforms.Num())
	{
		FString PlatformArrayStr = UFlibPatchParserHelper::GetTargetPlatformsStr(Platforms);
		if(!PlatformArrayStr.IsEmpty())
		{
			Result = FString::Printf(TEXT("-TargetPlatform=%s"),*PlatformArrayStr);
		}
	}
	return Result;
}

void UFlibPatchParserHelper::SetPropertyTransient(UStruct* Struct,const FString& PropertyName,bool bTransient)
{
	for(TFieldIterator<FProperty> PropertyIter(Struct);PropertyIter;++PropertyIter)
	{
		FProperty* PropertyIns = *PropertyIter;
		if(PropertyName.Equals(*PropertyIns->GetName(),ESearchCase::IgnoreCase))
		{
			if(bTransient)
			{
				PropertyIns->SetPropertyFlags(CPF_Transient);
			}
			else
			{
				PropertyIns->ClearPropertyFlags(CPF_Transient);
			}
			break;
		}
	}
}

FString UFlibPatchParserHelper::MergeOptionsAsCmdline(const TArray<FString>& InOptions)
{
	FString Cmdline;
	for(const auto& Option:InOptions)
	{
		FString InOption = Option;
		while(InOption.RemoveFromStart(TEXT(" "))){}
		while(InOption.RemoveFromEnd(TEXT(" "))){}
		if(!InOption.IsEmpty())
		{
			Cmdline += FString::Printf(TEXT("%s "),*InOption);
		}
	}
	Cmdline.RemoveFromEnd(TEXT(" "));
	return Cmdline;
};

FString UFlibPatchParserHelper::GetPlatformsStr(TArray<ETargetPlatform> Platforms)
{
	FString result;
	for(auto Platform:Platforms)
	{
		FString PlatformStr = THotPatcherTemplateHelper::GetEnumNameByValue(Platform,false);
		result+=FString::Printf(TEXT("%s,"),*PlatformStr);
	}
	result.RemoveFromEnd(TEXT(","));
	return result;
}


bool UFlibPatchParserHelper::GetCmdletBoolValue(const FString& Token, bool& OutValue)
{
	FString bTokenValue;
	bool bHasToken = FParse::Value(FCommandLine::Get(), *Token.ToLower(), bTokenValue);
	if(bHasToken)
	{
		if(bTokenValue.Equals(TEXT("true"),ESearchCase::IgnoreCase))
		{
			OutValue = true;
		}
		if(bTokenValue.Equals(TEXT("false"),ESearchCase::IgnoreCase))
		{
			OutValue = false;
		}
	}
	return bHasToken;
}

FString UFlibPatchParserHelper::ReplacePakRegular(const FReplacePakRegular& RegularConf, const FString& InRegular)
{
	struct FResularOperator
	{
		FResularOperator(const FString& InName,TFunction<FString(void)> InOperator)
			:Name(InName),Do(InOperator){}
		FString Name;
		TFunction<FString(void)> Do;
	};
	
	TArray<FResularOperator> RegularOpList;
	RegularOpList.Emplace(TEXT("{VERSION}"),[&RegularConf]()->FString{return RegularConf.VersionId;});
	RegularOpList.Emplace(TEXT("{BASEVERSION}"),[&RegularConf]()->FString{return RegularConf.BaseVersionId;});
	RegularOpList.Emplace(TEXT("{PLATFORM}"),[&RegularConf]()->FString{return RegularConf.PlatformName;});
	RegularOpList.Emplace(TEXT("{CHUNKNAME}"),[&RegularConf,InRegular]()->FString
	{
		if(InRegular.Contains(TEXT("{VERSION}")) &&
			InRegular.Contains(TEXT("{CHUNKNAME}")) &&
			RegularConf.VersionId.Equals(RegularConf.ChunkName))
		{
			return TEXT("");
		}
		else
		{
			return RegularConf.ChunkName;
		}
	});
	
	auto CustomPakNameRegular = [](const TArray<FResularOperator>& Operators,const FString& Regular)->FString
	{
		FString Result = Regular;
		for(auto& Operator:Operators)
		{
			Result = Result.Replace(*Operator.Name,*(Operator.Do()));
		}
		auto ReplaceDoubleLambda = [](FString& Src,const FString& From,const FString& To)
		{
			while(Src.Contains(From))
			{
				Src = Src.Replace(*From,*To);
			}
		};
		ReplaceDoubleLambda(Result,TEXT("__"),TEXT("_"));
		ReplaceDoubleLambda(Result,TEXT("--"),TEXT("-"));
		return Result;
	};
	
	return CustomPakNameRegular(RegularOpList,InRegular);
}
