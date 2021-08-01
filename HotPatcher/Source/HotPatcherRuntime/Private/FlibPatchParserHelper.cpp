// Fill out your copyright notice in the Description page of Project Settings.

// project header
#include "FlibPatchParserHelper.h"
#include "FLibAssetManageHelperEx.h"
#include "Struct/AssetManager/FFileArrayDirectoryVisitor.hpp"
#include "FLibAssetManageHelperEx.h"
#include "Flib/FLibAssetManageHelperEx.h"
#include "HotPatcherLog.h"
#include "CreatePatch/FExportPatchSettings.h"

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
#include "Misc/Paths.h"
#include "Kismet/KismetStringLibrary.h"

TArray<FString> UFlibPatchParserHelper::GetAvailableMaps(FString GameName, bool IncludeEngineMaps, bool IncludePluginMaps, bool Sorted)
{
	const FString WildCard = FString::Printf(TEXT("*%s"), *FPackageName::GetMapPackageExtension());
	TArray<FString> AllMaps;
	TMap<FString,FString> AllEnableModules;

	UFLibAssetManageHelperEx::GetAllEnabledModuleName(AllEnableModules);

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
						FString BelongModuneName = UFLibAssetManageHelperEx::GetAssetBelongModuleName(NewAssetItem);
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
				UFlibPatchParserHelper::GetAllExFilesByPlatform(InBaseVersion.PlatformAssets[Platform],false),
				UFlibPatchParserHelper::GetAllExFilesByPlatform(InNewVersion.PlatformAssets[Platform]),
				Platform
			));
		}
		else
		{
			FPlatformExternFiles EmptyBase;
			EmptyBase.Platform = Platform;
			OutDiff.Add(Platform,ParserDiffPlatformExFileLambda(
				EmptyBase,
                UFlibPatchParserHelper::GetAllExFilesByPlatform(InNewVersion.PlatformAssets[Platform],true),
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
                UFlibPatchParserHelper::GetAllExFilesByPlatform(InBaseVersion.PlatformAssets[Platform]),
                EmptyNew,
                Platform
            ));
		}
	}
	return true;
}

FPlatformExternFiles UFlibPatchParserHelper::GetAllExFilesByPlatform(
	const FPlatformExternAssets& InPlatformConf,bool InGeneratedHash)
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
			result.ExternFiles.Add(CurrentFile);
		}
	}
	if (InGeneratedHash)
	{
		for (auto& ExFile : result.ExternFiles)
		{
			ExFile.GenerateFileHash();
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
	if (UFLibAssetManageHelperEx::IsValidPlatform(InPlatformName))
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
	if (UFLibAssetManageHelperEx::IsValidPlatform(InPlatformName))
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

	if (UFLibAssetManageHelperEx::IsValidPlatform(InPlatformName))
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

bool UFlibPatchParserHelper::ConvNotAssetFileToExFile(const FString& InProjectDir, const FString& InPlatformName, const FString& InCookedFile, FExternFileInfo& OutExFile)
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

		OutExFile.FilePath.FilePath = InCookedFile;
		OutExFile.MountPath = AssetFileRelativeCookPath;
		OutExFile.GetFileHash();
		bRunStatus = true;
	}
	return bRunStatus;
}

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
		if (UFLibAssetManageHelperEx::GetPluginModuleAbsDir(Plugin->GetName(), PluginAbsPath))
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
		
		FPaths::MakeStandardFilename(DirAbsPath);
		if (!DirAbsPath.IsEmpty() && FPaths::DirectoryExists(DirAbsPath))
		{
			TArray<FString> DirectoryAllFiles;
			if (UFLibAssetManageHelperEx::FindFilesRecursive(DirAbsPath, DirectoryAllFiles, true))
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
					if (!result.Contains(CurrentFile))
						result.Add(CurrentFile);

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
		CurrentFile.mAssetType = TEXT("ExternalFile");
		CurrentFile.mPackagePath = File.MountPath;
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


TArray<FExternFileInfo> UFlibPatchParserHelper::GetInternalFilesAsExFiles(const FPakInternalInfo& InPakInternalInfo, const FString& InPlatformName)
{
	TArray<FExternFileInfo> resultFiles;

	TArray<FString> AllNeedPakInternalCookedFiles;

	FString ProjectAbsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	AllNeedPakInternalCookedFiles.Append(UFlibPatchParserHelper::GetCookedFilesByPakInternalInfo(InPakInternalInfo, InPlatformName));

	for (const auto& File : AllNeedPakInternalCookedFiles)
	{
		FExternFileInfo CurrentFile;
		UFlibPatchParserHelper::ConvNotAssetFileToExFile(ProjectAbsDir,InPlatformName, File, CurrentFile);
		resultFiles.Add(CurrentFile);
	}

	return resultFiles;
}

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
			Result.AddUnique(Filter.Path);
		}
	}
	return Result;
}

// TArray<FExternFileInfo> UFlibPatchParserHelper::GetExternFilesFromChunk(const FChunkInfo& InChunk,TArray<ETargetPlatform> InTargetPlatforms, bool bCalcHash)
// {
// 	TArray<FExternFileInfo> AllExternFiles = UFlibPatchParserHelper::ParserExDirectoryAsExFiles(InChunk.AddExternDirectoryToPak);
//
// 	for (auto& ExFile : InChunk.AddExternFileToPak)
// 	{
// 		if (!AllExternFiles.Contains(ExFile))
// 		{
// 			AllExternFiles.Add(ExFile);
// 		}
// 	}
// 	if (bCalcHash)
// 	{
// 		for (auto& ExFile : AllExternFiles)
// 		{
// 			ExFile.GenerateFileHash();
// 		}
// 	}
// 	return AllExternFiles;
// }

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
	const FPatchVersionDiff& DiffInfo,
	const FChunkInfo& Chunk,
	TArray<ETargetPlatform> Platforms,
	TMap<FString,FAssetDependenciesInfo>& ScanedCaches
	)
{
	FChunkAssetDescribe ChunkAssetDescribe;
	// Collect Chunk Assets
	{

		auto GetAssetFilterPaths = [](const TArray<FDirectoryPath>& InFilters)->TArray<FString>
		{
			TArray<FString> Result;
			for (const auto& Filter : InFilters)
			{
				if (!Filter.Path.IsEmpty())
				{
					Result.AddUnique(Filter.Path);
				}
			}
			return Result;
		};
		FAssetDependenciesInfo SpecifyDependAssets;

		auto GetSpecifyAssets = [&SpecifyDependAssets,&ScanedCaches](const TArray<FPatcherSpecifyAsset>& SpecifyAssets)->TArray<FString>
		{
			TArray<FString> result;
			for (const auto& Assset : SpecifyAssets)
			{
				FString CurrentAssetLongPackageName = Assset.Asset.GetLongPackageName();
				result.Add(CurrentAssetLongPackageName);
				if (Assset.bAnalysisAssetDependencies)
				{
					UFLibAssetManageHelperEx::GetAssetDependencies(CurrentAssetLongPackageName,Assset.AssetRegistryDependencyTypes, SpecifyDependAssets,ScanedCaches);
				}
			}
			return result;
		};

		TArray<FString> AssetFilterPaths = GetAssetFilterPaths(Chunk.AssetIncludeFilters);
		AssetFilterPaths.Append(GetSpecifyAssets(Chunk.IncludeSpecifyAssets));
		AssetFilterPaths.Append(UFLibAssetManageHelperEx::GetAssetLongPackageNameByAssetDependenciesInfo(SpecifyDependAssets));

		const FAssetDependenciesInfo& AddAssetsRef = DiffInfo.AssetDiffInfo.AddAssetDependInfo;
		const FAssetDependenciesInfo& ModifyAssetsRef = DiffInfo.AssetDiffInfo.ModifyAssetDependInfo;


		auto CollectChunkAssets = [](const FAssetDependenciesInfo& SearchBase, const TArray<FString>& SearchFilters)->FAssetDependenciesInfo
		{
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

		ChunkAssetDescribe.Assets = UFLibAssetManageHelperEx::CombineAssetDependencies(CollectChunkAssets(AddAssetsRef, AssetFilterPaths), CollectChunkAssets(ModifyAssetsRef, AssetFilterPaths));
	}

	auto CoolectPlatformExFiles = [](const FPatchVersionDiff& DiffInfo,const auto& Chunk,ETargetPlatform Platform)->TArray<FExternFileInfo>
	{
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
		auto CollectExtenFilesLambda = [&AllFiles](const TArray<FExternFileInfo>& SearchBase, const TSet<FString>& Filters)
		{
			for (const auto& Filter : Filters)
			{
				for (const auto& SearchItem : SearchBase)
				{
					if (SearchItem.FilePath.FilePath.StartsWith(Filter))
					{
						AllFiles.Add(SearchItem);
					}
				}
			}
		};
		CollectExtenFilesLambda(AddFilesRef, AllSearchFileFilter);
		CollectExtenFilesLambda(ModifyFilesRef, AllSearchFileFilter);
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
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
	const FExportPatchSettings* PatcheSettings
)
{
	TArray<FString> ChunkPakCommands;
	{
		TArray<FPakCommand> ChunkPakCommands_r = UFlibPatchParserHelper::CollectPakCommandByChunk(DiffInfo, Chunk, PlatformName,/* PakOptions,*/ScanedCaches,PatcheSettings);
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
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
	const FExportPatchSettings* PatcheSettings
)
{
	auto CollectPakCommandsByChunkLambda = [&ScanedCaches,PatcheSettings](const FPatchVersionDiff& DiffInfo, const FChunkInfo& Chunk, const FString& PlatformName/*, const TArray<FString>& PakOptions*/)->TArray<FPakCommand>
	{
		ETargetPlatform Platform;
		UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
		TArray<ETargetPlatform> CollectPlatforms = {ETargetPlatform::AllPlatforms};
		CollectPlatforms.AddUnique(Platform);
		FChunkAssetDescribe ChunkAssetsDescrible = UFlibPatchParserHelper::CollectFChunkAssetsDescribeByChunk(DiffInfo, Chunk ,CollectPlatforms,ScanedCaches);

		TArray<FPakCommand> PakCommands;

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
		
		auto ReceivePakCommandAssetLambda = [&PakCommands](const TArray<FString>& InPakCommand,const TArray<FString>& InIoStoreCommand, const FString& InMountPath,const FString& InLongPackageName)
		{
			FPakCommand CurrentPakCommand;
			CurrentPakCommand.PakCommands = InPakCommand;
			CurrentPakCommand.IoStoreCommands = InIoStoreCommand;
			CurrentPakCommand.MountPath = InMountPath;
			CurrentPakCommand.AssetPackage = InLongPackageName;
			PakCommands.Add(CurrentPakCommand);
		};
		
		// Collect Chunk Assets
		{
			FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
			TArray<FString> AssetsPakCommands;
			UFLibAssetManageHelperEx::MakePakCommandFromAssetDependencies(
				ProjectDir,
				PlatformName,
				ChunkAssetsDescrible.Assets,
				// PakOptions,
				AssetsPakCommands,
				ReceivePakCommandAssetLambda,
				IsIoStoreAssetsLambda
			);
			
		}

		auto ReceivePakCommandExFilesLambda = [&PakCommands](const FPakCommand& InCommand)
		{
			PakCommands.Emplace(InCommand);
		};
		
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

				PakCommands.Add(CurrentPakCommand);
			}
		}
		// internal files
		// TArray<FString> NotCompressOptions = PakOptions;
		// if(NotCompressOptions.Contains(TEXT("-compress")))
		// 	NotCompressOptions.Remove(TEXT("-compress"));
		UFlibPatchParserHelper::GetPakCommandsFromInternalInfo(ChunkAssetsDescrible.InternalFiles, PlatformName,/* NotCompressOptions,*/ ReceivePakCommandExFilesLambda);
		return PakCommands;
	};

	return CollectPakCommandsByChunkLambda(DiffInfo, Chunk, PlatformName/*, PakOptions*/);
}

FPatchVersionDiff UFlibPatchParserHelper::DiffPatchVersionWithPatchSetting(const FExportPatchSettings& PatchSetting, const FHotPatcherVersion& Base, const FHotPatcherVersion& New)
{
	FPatchVersionDiff VersionDiffInfo;
	FAssetDependenciesInfo BaseVersionAssetDependInfo = Base.AssetInfo;
	FAssetDependenciesInfo CurrentVersionAssetDependInfo = New.AssetInfo;

	UFlibPatchParserHelper::DiffVersionAssets(
		CurrentVersionAssetDependInfo,
		BaseVersionAssetDependInfo,
		VersionDiffInfo.AssetDiffInfo.AddAssetDependInfo,
		VersionDiffInfo.AssetDiffInfo.ModifyAssetDependInfo,
		VersionDiffInfo.AssetDiffInfo.DeleteAssetDependInfo
	);

	UFlibPatchParserHelper::DiffVersionAllPlatformExFiles(Base,New,VersionDiffInfo.PlatformExternDiffInfo);

	if(PatchSetting.GetIgnoreDeletionModulesAsset().Num())
	{
		for(const auto& ModuleName:PatchSetting.GetIgnoreDeletionModulesAsset())
		{
			VersionDiffInfo.AssetDiffInfo.DeleteAssetDependInfo.AssetsDependenciesMap.Remove(ModuleName);
		}
	}
	
	if(PatchSetting.IsRecursiveWidgetTree())
	{
		UFlibPatchParserHelper::AnalysisWidgetTree(VersionDiffInfo);
	}
	
	if(PatchSetting.IsForceSkipContent())
	{
		TArray<FString> AllSkipContents;
		AllSkipContents.Append(PatchSetting.GetForceSkipContentStrRules());
		AllSkipContents.Append(PatchSetting.GetForceSkipAssetsStr());
		UFlibPatchParserHelper::ExcludeContentForVersionDiff(VersionDiffInfo,AllSkipContents);
	}
	// clean deleted asset info in patch
	if(PatchSetting.IsIgnoreDeleatedAssetsInfo())
	{
		UE_LOG(LogHotPatcher,Display,TEXT("ignore deleted assets info in patch..."));
		VersionDiffInfo.AssetDiffInfo.DeleteAssetDependInfo.AssetsDependenciesMap.Empty();
		if(VersionDiffInfo.PlatformExternDiffInfo.Contains(ETargetPlatform::AllPlatforms))
		{
			VersionDiffInfo.PlatformExternDiffInfo.Find(ETargetPlatform::AllPlatforms)->DeleteExternalFiles.Empty();
		}
		for(const auto& Platform:PatchSetting.GetPakTargetPlatforms())
		{
			VersionDiffInfo.PlatformExternDiffInfo.Find(Platform)->DeleteExternalFiles.Empty();
		}
	}
	
	return VersionDiffInfo;
}

FHotPatcherVersion UFlibPatchParserHelper::ExportReleaseVersionInfo(
	const FString& InVersionId,
	const FString& InBaseVersion,
	const FString& InDate,
	const TArray<FString>& InIncludeFilter,
	const TArray<FString>& InIgnoreFilter,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	const TArray<FPatcherSpecifyAsset>& InIncludeSpecifyAsset,
	// const TArray<FExternAssetFileInfo>& InAllExternFiles,
	const TArray<FPlatformExternAssets>& AddToPlatformExFiles,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
	bool InIncludeHasRefAssetsOnly,
	bool bInAnalysisFilterDepend
)
{
	FHotPatcherVersion ExportVersion;
	{
		ExportVersion.VersionId = InVersionId;
		ExportVersion.Date = InDate;
		ExportVersion.BaseVersionId = InBaseVersion;
	}


	// add Include Filter
	{
		TArray<FString> AllIncludeFilter;
		for (const auto& Filter : InIncludeFilter)
		{
			AllIncludeFilter.AddUnique(Filter);
			ExportVersion.IncludeFilter.AddUnique(Filter);
		}
	}
	// add Ignore Filter
	{
		TArray<FString> AllIgnoreFilter;
		for (const auto& Filter : InIgnoreFilter)
		{
			AllIgnoreFilter.AddUnique(Filter);
			ExportVersion.IgnoreFilter.AddUnique(Filter);
		}
	}
	ExportVersion.bIncludeHasRefAssetsOnly = InIncludeHasRefAssetsOnly;
	ExportVersion.IncludeSpecifyAssets = InIncludeSpecifyAsset;

	TArray<FAssetDetail> FilterAssetList;
	{
		TArray<FAssetDetail> AllNeedPakRefAssets;
		{
			TArray<FAssetDetail> AllAssets;
			UFLibAssetManageHelperEx::GetAssetsList(ExportVersion.IncludeFilter, AssetRegistryDependencyTypes, AllAssets,ScanedCaches);
			if (InIncludeHasRefAssetsOnly)
			{
				TArray<FAssetDetail> AllDontHasRefAssets;
				UFLibAssetManageHelperEx::FilterNoRefAssetsWithIgnoreFilter(AllAssets, ExportVersion.IgnoreFilter, AllNeedPakRefAssets, AllDontHasRefAssets);
			}
			else
			{
				AllNeedPakRefAssets = AllAssets;
			}
		}
		// 剔除ignore filter中指定的资源
		if (ExportVersion.IgnoreFilter.Num() > 0)
		{
			for (const auto& AssetDetail : AllNeedPakRefAssets)
			{
				bool bIsIgnore = false;
				for (const auto& IgnoreFilter : ExportVersion.IgnoreFilter)
				{
					if (AssetDetail.mPackagePath.StartsWith(IgnoreFilter))
					{
						bIsIgnore = true;
						break;
					}
				}
				if (!bIsIgnore)
				{
					FilterAssetList.Add(AssetDetail);
				}
			}
		}
		else
		{
			FilterAssetList = AllNeedPakRefAssets;
		}

	}

	auto AnalysisAssetDependency = [&ScanedCaches](const TArray<FAssetDetail>& InAssetDetail, const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes, bool bInAnalysisDepend)->FAssetDependenciesInfo
	{
		FAssetDependenciesInfo RetAssetDepend;
		if (InAssetDetail.Num())
		{
			UFLibAssetManageHelperEx::CombineAssetsDetailAsFAssetDepenInfo(InAssetDetail, RetAssetDepend);

			if (bInAnalysisDepend)
			{
				FAssetDependenciesInfo AssetDependencies;
				UFLibAssetManageHelperEx::GetAssetListDependenciesForAssetDetail(InAssetDetail,AssetRegistryDependencyTypes, AssetDependencies,ScanedCaches);

				RetAssetDepend = UFLibAssetManageHelperEx::CombineAssetDependencies(RetAssetDepend, AssetDependencies);
			}
		}
		return RetAssetDepend;
	};

	// 分析过滤器中指定的资源依赖
	FAssetDependenciesInfo FilterAssetDependencies = AnalysisAssetDependency(FilterAssetList, AssetRegistryDependencyTypes, bInAnalysisFilterDepend);

	// Specify Assets
	FAssetDependenciesInfo SpecifyAssetDependencies;
	{
		for (const auto& SpecifyAsset : InIncludeSpecifyAsset)
		{
			FString AssetLongPackageName = SpecifyAsset.Asset.GetLongPackageName();
			FAssetDetail AssetDetail;
			if (UFLibAssetManageHelperEx::GetSpecifyAssetDetail(AssetLongPackageName, AssetDetail))
			{
				FAssetDependenciesInfo CurrentAssetDependencies = AnalysisAssetDependency(TArray<FAssetDetail>{AssetDetail}, SpecifyAsset.AssetRegistryDependencyTypes, SpecifyAsset.bAnalysisAssetDependencies);
				SpecifyAssetDependencies = UFLibAssetManageHelperEx::CombineAssetDependencies(SpecifyAssetDependencies, CurrentAssetDependencies);
			}
		}
	}

	ExportVersion.AssetInfo = UFLibAssetManageHelperEx::CombineAssetDependencies(FilterAssetDependencies, SpecifyAssetDependencies);

	// for (const auto& File : InAllExternFiles)
	// {
	// 	if (!ExportVersion.ExternalFiles.Contains(File.FilePath.FilePath))
	// 	{
	// 		ExportVersion.ExternalFiles.Add(File.MountPath, File);
	// 	}
	// }

	for(const auto& PlatformExInfo:AddToPlatformExFiles)
	{
		FPlatformExternFiles PlatformFileInfo = UFlibPatchParserHelper::GetAllExFilesByPlatform(PlatformExInfo);
		FPlatformExternAssets PlatformExternAssets;
		PlatformExternAssets.TargetPlatform = PlatformExInfo.TargetPlatform;
		PlatformExternAssets.AddExternFileToPak = PlatformFileInfo.ExternFiles;
		ExportVersion.PlatformAssets.Add(PlatformExInfo.TargetPlatform,PlatformExternAssets);
	}
	return ExportVersion;
}

FHotPatcherVersion UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
	const FString& InVersionId,
	const FString& InBaseVersion,
	const FString& InDate,
	const FChunkInfo& InChunkInfo,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches,
	bool InIncludeHasRefAssetsOnly /*= false */,
	bool bInAnalysisFilterDepend /* = true*/)
{
	return UFlibPatchParserHelper::ExportReleaseVersionInfo(
        InVersionId,
        InBaseVersion,
        InDate,
        UFlibPatchParserHelper::GetDirectoryPaths(InChunkInfo.AssetIncludeFilters),
        UFlibPatchParserHelper::GetDirectoryPaths(InChunkInfo.AssetIgnoreFilters),
        InChunkInfo.AssetRegistryDependencyTypes,
        InChunkInfo.IncludeSpecifyAssets,
        InChunkInfo.AddExternAssetsToPlatform,
        ScanedCaches,
        InIncludeHasRefAssetsOnly,
        bInAnalysisFilterDepend
    );
}


FChunkAssetDescribe UFlibPatchParserHelper::DiffChunkWithPatchSetting(
	const FExportPatchSettings& PatchSetting,
	const FChunkInfo& CurrentVersionChunk,
	const FChunkInfo& TotalChunk,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	FHotPatcherVersion TotalChunkVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT(""),
		TEXT(""),
		TEXT(""),
		TotalChunk,
		ScanedCaches,
		PatchSetting.IsIncludeHasRefAssetsOnly(),
		TotalChunk.bAnalysisFilterDependencies
	);

	return UFlibPatchParserHelper::DiffChunkByBaseVersionWithPatchSetting(PatchSetting, CurrentVersionChunk ,TotalChunk, TotalChunkVersion,ScanedCaches);
}

FChunkAssetDescribe UFlibPatchParserHelper::DiffChunkByBaseVersionWithPatchSetting(
	const FExportPatchSettings& PatchSetting,
	const FChunkInfo& CurrentVersionChunk,
	const FChunkInfo& TotalChunk,
	const FHotPatcherVersion& BaseVersion,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	FChunkAssetDescribe result;
	FHotPatcherVersion CurrentVersion = UFlibPatchParserHelper::ExportReleaseVersionInfoByChunk(
		TEXT(""),
		TEXT(""),
		TEXT(""),
		CurrentVersionChunk,
		ScanedCaches,
		PatchSetting.IsIncludeHasRefAssetsOnly(),
		CurrentVersionChunk.bAnalysisFilterDependencies
	);
	FPatchVersionDiff ChunkDiffInfo = DiffPatchVersionWithPatchSetting(PatchSetting, BaseVersion, CurrentVersion);
	
	result.Assets = UFLibAssetManageHelperEx::CombineAssetDependencies(ChunkDiffInfo.AssetDiffInfo.AddAssetDependInfo, ChunkDiffInfo.AssetDiffInfo.ModifyAssetDependInfo);

	TArray<ETargetPlatform> Platforms;
	ChunkDiffInfo.PlatformExternDiffInfo.GetKeys(Platforms);
	for(auto Platform:Platforms)
	{
		FPlatformExternFiles PlatformFiles;
		PlatformFiles.Platform = Platform;
		PlatformFiles.ExternFiles = ChunkDiffInfo.PlatformExternDiffInfo.Find(Platform)->AddExternalFiles;
		PlatformFiles.ExternFiles.Append(ChunkDiffInfo.PlatformExternDiffInfo.Find(Platform)->ModifyExternalFiles);
		result.AllPlatformExFiles.Add(Platform,PlatformFiles);	
	}
	
	result.InternalFiles.bIncludeAssetRegistry = CurrentVersionChunk.InternalFiles.bIncludeAssetRegistry != TotalChunk.InternalFiles.bIncludeAssetRegistry;
	result.InternalFiles.bIncludeGlobalShaderCache = CurrentVersionChunk.InternalFiles.bIncludeGlobalShaderCache != TotalChunk.InternalFiles.bIncludeGlobalShaderCache;
	result.InternalFiles.bIncludeShaderBytecode = CurrentVersionChunk.InternalFiles.bIncludeShaderBytecode != TotalChunk.InternalFiles.bIncludeShaderBytecode;
	result.InternalFiles.bIncludeEngineIni = CurrentVersionChunk.InternalFiles.bIncludeEngineIni != TotalChunk.InternalFiles.bIncludeEngineIni;
	result.InternalFiles.bIncludePluginIni = CurrentVersionChunk.InternalFiles.bIncludePluginIni != TotalChunk.InternalFiles.bIncludePluginIni;
	result.InternalFiles.bIncludeProjectIni = CurrentVersionChunk.InternalFiles.bIncludeProjectIni != TotalChunk.InternalFiles.bIncludeProjectIni;

	return result;
}

TArray<FString> UFlibPatchParserHelper::GetPakCommandStrByCommands(const TArray<FPakCommand>& PakCommands,const TArray<FReplaceText>& InReplaceTexts,bool bIoStore)
{
	TArray<FString> ResultPakCommands;
	{
		for (const auto& PakCommand : PakCommands)
		{
			const TArray<FString>& PakCommandOriginTexts = bIoStore?PakCommand.GetIoStoreCommands():PakCommand.GetPakCommands();
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
					ResultPakCommands.Add(PakCommandTargetText);
				}
			}
			else
			{
				ResultPakCommands.Append(PakCommandOriginTexts);
			}
		}
	}
	return ResultPakCommands;
}



FHotPatcherAssetDependency UFlibPatchParserHelper::GetAssetRelatedInfo(
	const FAssetDetail& InAsset,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	FHotPatcherAssetDependency Dependency;
	Dependency.Asset = InAsset;
	FString LongPackageName;
	if (UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(InAsset.mPackagePath, LongPackageName))
	{
		TArray<EAssetRegistryDependencyType::Type> SearchAssetDepTypes;
		for (const auto& Type : AssetRegistryDependencyTypes)
		{
			SearchAssetDepTypes.AddUnique(UFLibAssetManageHelperEx::ConvAssetRegistryDependencyToInternal(Type));
		}

		UFLibAssetManageHelperEx::GetAssetDependency(LongPackageName, AssetRegistryDependencyTypes,Dependency.AssetDependency, ScanedCaches,false);
		UFLibAssetManageHelperEx::GetAssetReference(InAsset, SearchAssetDepTypes, Dependency.AssetReference);

	}

	return Dependency;
}

TArray<FHotPatcherAssetDependency> UFlibPatchParserHelper::GetAssetsRelatedInfo(
	const TArray<FAssetDetail>& InAssets,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	TArray<FHotPatcherAssetDependency> AssetsDependency;

	for (const auto& Asset : InAssets)
	{
		AssetsDependency.Add(UFlibPatchParserHelper::GetAssetRelatedInfo(Asset, AssetRegistryDependencyTypes,ScanedCaches));
	}
	return AssetsDependency;
}

TArray<FHotPatcherAssetDependency> UFlibPatchParserHelper::GetAssetsRelatedInfoByFAssetDependencies(
	const FAssetDependenciesInfo& InAssetsDependencies,
	const TArray<EAssetRegistryDependencyTypeEx>& AssetRegistryDependencyTypes,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
)
{
	TArray<FAssetDetail> AssetsDetail;
	UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(InAssetsDependencies, AssetsDetail);
	return UFlibPatchParserHelper::GetAssetsRelatedInfo(AssetsDetail, AssetRegistryDependencyTypes,ScanedCaches);
}



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
		if (UFLibAssetManageHelperEx::ConvRelativeDirToAbsDir(CookFilter, FilterFullPath))
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

void UFlibPatchParserHelper::ExcludeContentForVersionDiff(FPatchVersionDiff& VersionDiff,const TArray<FString>& ExcludeRules)
{
	auto ExcludeEditorContent = [&ExcludeRules](TMap<FString,FAssetDependenciesDetail>& AssetCategorys)
	{
		TArray<FString> Keys;
		AssetCategorys.GetKeys(Keys);
		for(const auto& Key:Keys)
		{
			FAssetDependenciesDetail&  ModuleAssetList = *AssetCategorys.Find(Key);
			TArray<FString> AssetKeys;
			
			ModuleAssetList.AssetDependencyDetails.GetKeys(AssetKeys);
			for(const auto& AssetKey:AssetKeys)
			{
				bool customStartWith = false;
                for(const auto& Rule:ExcludeRules)
                {
                	if(AssetKey.StartsWith(Rule))
                	{
                		customStartWith = true;
                		break;
                	}
                }
                			
                if( customStartWith )
                {
                	ModuleAssetList.AssetDependencyDetails.Remove(AssetKey);
                }
			}
		}
	};

	ExcludeEditorContent(VersionDiff.AssetDiffInfo.AddAssetDependInfo.AssetsDependenciesMap);
	ExcludeEditorContent(VersionDiff.AssetDiffInfo.ModifyAssetDependInfo.AssetsDependenciesMap);
	ExcludeEditorContent(VersionDiff.AssetDiffInfo.DeleteAssetDependInfo.AssetsDependenciesMap);
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


#include "ShaderCodeLibrary.h"

void UFlibPatchParserHelper::ReloadShaderbytecode()
{
	FShaderCodeLibrary::OpenLibrary("Global", FPaths::ProjectContentDir());
	FShaderCodeLibrary::OpenLibrary(FApp::GetProjectName(), FPaths::ProjectContentDir());
}

bool UFlibPatchParserHelper::LoadShaderbytecode(const FString& LibraryName, const FString& LibraryDir)
{
	bool result = true;
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 23
	result = FShaderCodeLibrary::OpenLibrary(LibraryName, LibraryDir);
#else
	FShaderCodeLibrary::OpenLibrary(LibraryName, LibraryDir);
#endif
	return result;
}

FString UFlibPatchParserHelper::SerializeAssetsDependencyAsJsonString(
	const TArray<FHotPatcherAssetDependency>& InAssetsDependency)
{
	FString AssetsDependencyString;
	auto SerializeAssetsDependencyLambda = [](const TArray<FHotPatcherAssetDependency>& InAssetsDependency, TSharedPtr<FJsonObject>& OutJsonObject)->bool
	{
		if (!OutJsonObject.IsValid())
			OutJsonObject = MakeShareable(new FJsonObject);

		TArray<TSharedPtr<FJsonValue>> RefAssetsObjectList;
		for (const auto& AssetDependency: InAssetsDependency)
		{
			TSharedPtr<FJsonObject> CurrentAssetDependencyJsonObject;
			if(UFlibPatchParserHelper::TSerializeStructAsJsonObject(AssetDependency,CurrentAssetDependencyJsonObject))
				RefAssetsObjectList.Add(MakeShareable(new FJsonValueObject(CurrentAssetDependencyJsonObject)));
		}
		OutJsonObject->SetArrayField(TEXT("AssetsRelatedInfos"),RefAssetsObjectList);
		return true;
	};
	TSharedPtr<FJsonObject> SerializeObject;
	SerializeAssetsDependencyLambda(InAssetsDependency,SerializeObject);
				
	auto JsonWriter = TJsonWriterFactory<>::Create(&AssetsDependencyString);
	FJsonSerializer::Serialize(SerializeObject.ToSharedRef(), JsonWriter);
	return AssetsDependencyString;
}

bool UFlibPatchParserHelper::SerializePlatformPakInfoToString(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, FString& OutString)
{
	bool bRunStatus = false;
	TSharedPtr<FJsonObject> RootJsonObj = MakeShareable(new FJsonObject);

	bRunStatus = UFlibPatchParserHelper::SerializePlatformPakInfoToJsonObject(InPakFilesMap, RootJsonObj);

	auto JsonWriter = TJsonWriterFactory<TCHAR>::Create(&OutString);
	FJsonSerializer::Serialize(RootJsonObj.ToSharedRef(), JsonWriter);

	return bRunStatus;
}

bool UFlibPatchParserHelper::SerializePlatformPakInfoToJsonObject(const TMap<FString, TArray<FPakFileInfo>>& InPakFilesMap, TSharedPtr<FJsonObject>& OutJsonObject)
{
	bool bRunStatus = false;
	if (!OutJsonObject.IsValid())
	{
		OutJsonObject = MakeShareable(new FJsonObject);
	}

	// serialize
	{
		TArray<FString> PakPlatformKeys;
		InPakFilesMap.GetKeys(PakPlatformKeys);

		for (const auto& PakPlatformKey : PakPlatformKeys)
		{
			TSharedPtr<FJsonObject> CurrentPlatformPakJsonObj;
			TArray<TSharedPtr<FJsonValue>> CurrentPlatformPaksObjectList;
			for (const auto& Pak : *InPakFilesMap.Find(PakPlatformKey))
			{
				TSharedPtr<FJsonObject> CurrentPakJsonObj;
				if (UFlibPatchParserHelper::TSerializeStructAsJsonObject(Pak, CurrentPakJsonObj))
				{
					CurrentPlatformPaksObjectList.Add(MakeShareable(new FJsonValueObject(CurrentPakJsonObj)));
				}
			}
			
			OutJsonObject->SetArrayField(PakPlatformKey, CurrentPlatformPaksObjectList);
		}
		bRunStatus = true;
	}
	return bRunStatus;
}

TArray<FAssetDetail> UFlibPatchParserHelper::GetAllAssetDependencyDetails(
	const FAssetDetail& Asset,
	const TArray<EAssetRegistryDependencyTypeEx>& Types,
	const FString& AssetType,
	TMap<FString, FAssetDependenciesInfo>& ScanedCaches
	)
{
	TArray<FAssetDetail> result;
	TArray<FHotPatcherAssetDependency> AssetDeps = UFlibPatchParserHelper::GetAssetsRelatedInfo(TArray<FAssetDetail>{Asset}, Types,ScanedCaches);
	for(const auto& AssetDependency:AssetDeps)
	{
		for(const auto& AssetRederenceItem:AssetDependency.AssetReference)
		{
			if(AssetType.IsEmpty() || AssetRederenceItem.mAssetType.Equals(AssetType,ESearchCase::IgnoreCase))
			{
				TArray<FAssetDetail> CurrentAssetDepAssetList = UFlibPatchParserHelper::GetAllAssetDependencyDetails(AssetRederenceItem,Types,AssetType,ScanedCaches);
				CurrentAssetDepAssetList.AddUnique(AssetRederenceItem);
				for(const auto& AssetItem:CurrentAssetDepAssetList)
				{
					result.AddUnique(AssetItem);
				}
            }
		}
	}
	return result;
}

void UFlibPatchParserHelper::AnalysisWidgetTree(FPatchVersionDiff& PakDiff,int32 flags)
{
	TArray<FAssetDetail> AnalysisAssets;
	if(flags & 0x1)
	{
		TArray<FAssetDetail> AddAssets;
		UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(PakDiff.AssetDiffInfo.AddAssetDependInfo,AddAssets);
		AnalysisAssets.Append(AddAssets);
		
	}
	if(flags & 0x2)
	{
		TArray<FAssetDetail> ModifyAssets;
		UFLibAssetManageHelperEx::GetAssetDetailsByAssetDependenciesInfo(PakDiff.AssetDiffInfo.ModifyAssetDependInfo,ModifyAssets);
		AnalysisAssets.Append(ModifyAssets);
	}
	TArray<EAssetRegistryDependencyTypeEx> AssetRegistryDepTypes {EAssetRegistryDependencyTypeEx::Hard};
	FString AssetType = TEXT("WidgetBlueprint");

	auto AssetsIsExist = [&PakDiff](const FAssetDetail& Asset)->bool
	{
		bool result = false;
		FString ModuleName = UFLibAssetManageHelperEx::GetAssetBelongModuleName(Asset.mPackagePath);
		FString LongPackageName;
		UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(Asset.mPackagePath,LongPackageName);
		if(PakDiff.AssetDiffInfo.ModifyAssetDependInfo.AssetsDependenciesMap.Find(ModuleName))
		{
			if(PakDiff.AssetDiffInfo.ModifyAssetDependInfo.AssetsDependenciesMap.Find(ModuleName)->AssetDependencyDetails.Find(LongPackageName))
			{
				result = true;
			}
		}
		return result;
	};
	TArray<EAssetRegistryDependencyType::Type> SearchType{EAssetRegistryDependencyType::Hard};
	
	for(const auto& OriginAsset:AnalysisAssets)
	{
		if(OriginAsset.mAssetType.Equals(AssetType))
		{
			// if asset is existed
			if(AssetsIsExist(OriginAsset))
			{
				continue;
			}
			TArray<FAssetDetail> CurrentAssetsRef;
			UFLibAssetManageHelperEx::GetAssetReferenceRecursively(OriginAsset, SearchType, TArray<FString>{AssetType}, CurrentAssetsRef);
			UE_LOG(LogHotPatcher,Display,TEXT("Reference %s Widgets:"),*OriginAsset.mPackagePath);
			// TArray<FAssetDetail> CurrentAssetDeps = UFlibPatchParserHelper::GetAllAssetDependencyDetails(OriginAsset,AssetRegistryDepTypes,AssetType);
			for(const auto& Asset:CurrentAssetsRef)
			{
				if(!Asset.mAssetType.Equals(AssetType))
				{
					continue;
				}
				
				UE_LOG(LogHotPatcher,Display,TEXT("Widget: %s"),*Asset.mPackagePath);
				FString ModuleName = UFLibAssetManageHelperEx::GetAssetBelongModuleName(Asset.mPackagePath);
				FString LongPackageName;
				UFLibAssetManageHelperEx::ConvPackagePathToLongPackageName(Asset.mPackagePath,LongPackageName);
				if(PakDiff.AssetDiffInfo.ModifyAssetDependInfo.AssetsDependenciesMap.Find(ModuleName))
				{
					PakDiff.AssetDiffInfo.ModifyAssetDependInfo.AssetsDependenciesMap.Find(ModuleName)->AssetDependencyDetails.Add(LongPackageName,Asset);
				}
				else
				{
					FAssetDependenciesDetail AssetModuleDetail;
					AssetModuleDetail.ModuleCategory = ModuleName;
					AssetModuleDetail.AssetDependencyDetails.Add(LongPackageName,Asset);
					PakDiff.AssetDiffInfo.ModifyAssetDependInfo.AssetsDependenciesMap.Add(ModuleName,AssetModuleDetail);
				}
			}
		}
	}
}
#define ENGINEDIR_MARK TEXT("[ENGINEDIR]/")
#define ENGINE_CONTENT_DIR_MARK TEXT("[ENGINE_CONTENT_DIR]/")
#define PROJECTDIR_MARK TEXT("[PROJECTDIR]/")
#define PROJECT_CONTENT_DIR_MARK TEXT("[PROJECT_CONTENT_DIR]/")
#define PROJECT_SAVED_DIR_MARK TEXT("[PROJECT_SAVED_DIR]/")
#define PROJECT_CONFIG_DIR_MARK TEXT("[PROJECT_CONFIG_DIR]/")

TMap<FString, FString> UFlibPatchParserHelper::GetReplacePathMarkMap()
{
	TMap<FString,FString> MarkMap;
	MarkMap.Add(ENGINEDIR_MARK,FPaths::EngineDir());
	MarkMap.Add(ENGINE_CONTENT_DIR_MARK,FPaths::EngineContentDir());
	MarkMap.Add(PROJECTDIR_MARK,FPaths::ProjectDir());
	MarkMap.Add(PROJECT_CONTENT_DIR_MARK,FPaths::ProjectContentDir());
	MarkMap.Add(PROJECT_SAVED_DIR_MARK,FPaths::ProjectSavedDir());
	MarkMap.Add(PROJECT_CONFIG_DIR_MARK,FPaths::ProjectConfigDir());
	return MarkMap;
}

FString UFlibPatchParserHelper::ReplaceMarkPath(const FString& Src)
{
	TMap<FString,FString> MarkMap = UFlibPatchParserHelper::GetReplacePathMarkMap();
	auto ReplaceProjectDir = [&MarkMap](const FString& OriginDir)->FString
	{
		TArray<FString> MarkKeys;
		MarkMap.GetKeys(MarkKeys);
		
		FString result = OriginDir;
		for(const auto& Key:MarkKeys)
		{
			if(OriginDir.StartsWith(Key))
			{
				result = OriginDir;
				result.RemoveFromStart(Key,ESearchCase::IgnoreCase);
				result = FPaths::Combine(MarkMap[Key],result);
				break;
			}
		}

		FPaths::MakeStandardFilename(result);
		result = FPaths::ConvertRelativePathToFull(result);
		return result;
	};
	return ReplaceProjectDir(Src);
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


FHotPatcherVersion UFlibPatchParserHelper::MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion, FExportPatchSettings* InPatchSettings)
{
	FHotPatcherVersion BaseVersion = InBaseVersion;
	
	FPatchVersionDiff DiffInfo = UFlibPatchParserHelper::DiffPatchVersionWithPatchSetting(*InPatchSettings,BaseVersion, InCurrentVersion);
	return UFlibPatchParserHelper::MakeNewReleaseByDiff(InBaseVersion,DiffInfo, InPatchSettings);
}

FHotPatcherVersion UFlibPatchParserHelper::MakeNewReleaseByDiff(const FHotPatcherVersion& InBaseVersion,
	const FPatchVersionDiff& InDiff, FExportPatchSettings* InPatchSettings)
{
	FHotPatcherVersion BaseVersion = InBaseVersion;
	FHotPatcherVersion NewRelease;

	NewRelease.BaseVersionId = InBaseVersion.VersionId;
	NewRelease.Date = FDateTime::UtcNow().ToString();
	NewRelease.VersionId = InPatchSettings->VersionId;
	
	FAssetDependenciesInfo& BaseAssetInfoRef = BaseVersion.AssetInfo;
	// TMap<FString, FExternFileInfo>& BaseExternalFilesRef = BaseVersion.ExternalFiles;
	TMap<ETargetPlatform,FPlatformExternAssets>& BasePlatformAssetsRef = BaseVersion.PlatformAssets;

	// Modify Asset
	auto DeleteOldAssetLambda = [&BaseAssetInfoRef](const FAssetDependenciesInfo& InAssetDependenciesInfo)
	{
		for (const auto& AssetsModulePair : InAssetDependenciesInfo.AssetsDependenciesMap)
		{
			FAssetDependenciesDetail* NewReleaseModuleAssets = BaseAssetInfoRef.AssetsDependenciesMap.Find(AssetsModulePair.Key);

			for (const auto& NeedDeleteAsset : AssetsModulePair.Value.AssetDependencyDetails)
			{
				if (NewReleaseModuleAssets && NewReleaseModuleAssets->AssetDependencyDetails.Contains(NeedDeleteAsset.Key))
				{
					NewReleaseModuleAssets->AssetDependencyDetails.Remove(NeedDeleteAsset.Key);
				}
			}
		}
	};
	
	DeleteOldAssetLambda(InDiff.AssetDiffInfo.ModifyAssetDependInfo);
	if(InPatchSettings && !InPatchSettings->IsSaveDeletedAssetsToNewReleaseJson())
	{
		DeleteOldAssetLambda(InDiff.AssetDiffInfo.DeleteAssetDependInfo);
	}

	// Add Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, InDiff.AssetDiffInfo.AddAssetDependInfo);
	// modify Asset
	BaseAssetInfoRef = UFLibAssetManageHelperEx::CombineAssetDependencies(BaseAssetInfoRef, InDiff.AssetDiffInfo.ModifyAssetDependInfo);
	NewRelease.AssetInfo = BaseAssetInfoRef;

	// // external files
	// auto RemoveOldExternalFilesLambda = [&BaseExternalFilesRef](const TArray<FExternFileInfo>& InFiles)
	// {
	// 	for (const auto& File : InFiles)
	// 	{
	// 		if (BaseExternalFilesRef.Contains(File.FilePath.FilePath))
	// 		{
	// 			BaseExternalFilesRef.Remove(File.FilePath.FilePath);
	// 		}
	// 	}
	// };

	TArray<ETargetPlatform> DiffPlatforms;
	InDiff.PlatformExternDiffInfo.GetKeys(DiffPlatforms);

	for(auto Platform:DiffPlatforms)
	{
		FPlatformExternAssets AddPlatformFiles;
		AddPlatformFiles.TargetPlatform = Platform;
		AddPlatformFiles.AddExternFileToPak = InDiff.PlatformExternDiffInfo[Platform].AddExternalFiles;
		AddPlatformFiles.AddExternFileToPak.Append(InDiff.PlatformExternDiffInfo[Platform].ModifyExternalFiles);
		if(BasePlatformAssetsRef.Contains(Platform))
		{
			for(const auto& File:AddPlatformFiles.AddExternFileToPak)
			{
				if(BasePlatformAssetsRef[Platform].AddExternFileToPak.Contains(File))
				{
					BasePlatformAssetsRef[Platform].AddExternFileToPak.Remove(File);
				}
				BasePlatformAssetsRef[Platform].AddExternFileToPak.Add(File);
			}
		}else
		{
			BasePlatformAssetsRef.Add(Platform,AddPlatformFiles);
		}
	}
	// RemoveOldExternalFilesLambda(DiffInfo.ExternDiffInfo.ModifyExternalFiles);
	// DeleteOldExternalFilesLambda(DiffInfo.DeleteExternalFiles);

	NewRelease.PlatformAssets = BasePlatformAssetsRef;
	return NewRelease;
}



bool UFlibPatchParserHelper::IsUasset(const FString& InAsset)
{
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
	return IsUAssetLambda(InAsset);
}

FString UFlibPatchParserHelper::UAssetMountPathToPackagePath(const FString& InAssetMountPath)
{
	TMap<FString, FString> ReceiveModuleMap;
	UFLibAssetManageHelperEx::GetAllEnabledModuleName(ReceiveModuleMap);

	FString LongPackageName = InAssetMountPath;

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
	UFLibAssetManageHelperEx::ConvLongPackageNameToPackagePath(LongPackageName, LongPackagePath);
	return LongPackagePath;
}

