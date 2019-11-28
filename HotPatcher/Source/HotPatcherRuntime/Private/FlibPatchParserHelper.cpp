// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibPatchParserHelper.h"
#include "FlibSourceControlHelper.h"
#include "FlibAssetManageHelper.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/IPluginManager.h"

//FString UFlibPatchParserHelper::GetHotPatcherGitPath()
//{
//	return UFlibSourceControlHelper::GetGitBinary();
//}
//
//bool UFlibPatchParserHelper::DiffCommitVersion(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, TArray<FString>& OutAbsPathResault, TArray<FString>& OutErrorMessages)
//{
//	TArray<FString> OutResault;
//	bool bRunState = UFlibSourceControlHelper::DiffVersion(InGitBinaey, InRepoRoot, InBeginCommitHash, InEndCommitHash, OutResault,OutErrorMessages);
//
//	for (const auto& Item : OutResault)
//	{
//		FString ItemAbsPath = UFlibPatchParserHelper::ConvGitTrackFilePathToAbsPath(InRepoRoot,Item);
//		if (FPaths::FileExists(ItemAbsPath))
//		{
//			OutAbsPathResault.Add(UFlibAssetManageHelper::ConvPath_BackSlash2Slash(FPaths::Combine(InRepoRoot, Item)));
//			continue;
//		}
//		if (FPaths::DirectoryExists(ItemAbsPath))
//		{
//			FFillArrayDirectoryVisitor FileVisitor;
//			IFileManager::Get().IterateDirectoryRecursively(*Item, FileVisitor);
//			for (const auto& NewFile : FileVisitor.Files)
//			{
//				if (!OutAbsPathResault.Contains(NewFile))
//				{
//					OutAbsPathResault.Add(NewFile);
//				}
//			}
//		}
//	}
//
//	return bRunState;
//}
//
//
//bool UFlibPatchParserHelper::DiffCommitVersionAsAssetDependencies(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, FAssetDependenciesInfo& OutContentDiffInfo, FAssetDependenciesInfo& OutDiffAssetDependencies)
//{
//	{
//		OutContentDiffInfo = FAssetDependenciesInfo{};
//		OutDiffAssetDependencies = FAssetDependenciesInfo{};
//	}
//	TArray<FString> DiffAssetAbsList;
//	TArray<FString> DiffErrorMsg;
//	bool runState= UFlibPatchParserHelper::DiffCommitVersion(InGitBinaey, InRepoRoot, InBeginCommitHash, InEndCommitHash, DiffAssetAbsList, DiffErrorMsg);;
//	
//
//	if (runState)	
//	{
//		TArray<FString> AllEnableModules;
//		UFlibPatchParserHelper::GetAllEnabledModuleBaseDir(AllEnableModules);
//		TArray<FString> ContentOnlyAbsList;
//		TArray<FString> ContentOnlyPackageList;
//		TArray<FString> ContentOnlyShortNameList;
//		UFlibPatchParserHelper::FilterContentPathInArray(AllEnableModules, DiffAssetAbsList, ContentOnlyAbsList);
//		UFlibAssetManageHelper::ConvAssetPathListFromAbsToRelative(InRepoRoot, ContentOnlyAbsList, ContentOnlyPackageList);
//
//		// Remove asset package postfix
//		for (const auto& AssetPackageName : ContentOnlyPackageList)
//		{
//			FString convResult;
//			if (UFlibAssetManageHelper::RemovePackageAssetPostfix(AssetPackageName, convResult))
//			{
//				ContentOnlyShortNameList.Add(convResult);
//			}
//		}
//		// add to OutContentDiffInfo
//		{
//			for (const auto& AssetItem : ContentOnlyShortNameList)
//			{
//				FString AssetBelongModule = UFlibAssetManageHelper::GetAssetBelongModuleName(AssetItem);
//
//				if (OutContentDiffInfo.mDependencies.Contains(AssetBelongModule))
//				{
//					OutContentDiffInfo.mDependencies.Find(AssetBelongModule)->mDependAsset.Add(AssetItem);
//				}
//				else
//				{
//					OutContentDiffInfo.mDependencies.Add(AssetBelongModule, FAssetDependenciesDetail{ AssetBelongModule,TArray<FString>{AssetItem} });
//				}
//			}
//		}
//
//		FAssetDependenciesInfo OutResult;
//
//		for(const auto& AssetItem:ContentOnlyPackageList)
//		{
//			FAssetDependenciesInfo SignelAssetDep;
//			UFlibAssetManageHelper::GetAssetDependencies(AssetItem, SignelAssetDep);
//			OutDiffAssetDependencies = UFlibAssetManageHelper::CombineAssetDependencies(OutDiffAssetDependencies, SignelAssetDep);
//		}
//			
//	}
//	return runState;
//}
//
//bool UFlibPatchParserHelper::ParserDiffAssetDependencies(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, FAssetDependenciesInfo& OutDiffDependencies)
//{
//	FAssetDependenciesInfo resault;
//
//	TArray<FString> AllDifferenceAbsPathList;
//	TArray<FString> ErrorMsg;
//	bool bRunState = UFlibPatchParserHelper::DiffCommitVersion(InGitBinaey, InRepoRoot, InBeginCommitHash, InEndCommitHash, AllDifferenceAbsPathList, ErrorMsg);
//	if (bRunState)
//	{
//		TArray<FString> AllEnableModuleBaseDir;
//		UFlibPatchParserHelper::GetAllEnabledModuleBaseDir(AllEnableModuleBaseDir);
//		TArray<FString> FilteredContentAbsPathList;
//		UFlibPatchParserHelper::FilterContentPathInArray(AllEnableModuleBaseDir, AllDifferenceAbsPathList, FilteredContentAbsPathList);
//
//		TArray<FString> FilteredContentRelativePathList;
//		UFlibAssetManageHelper::ConvAssetPathListFromAbsToRelative(InRepoRoot, FilteredContentAbsPathList, FilteredContentRelativePathList);
//
//
//		for (const auto& AssetRelativePathItem : FilteredContentRelativePathList)
//		{
//			FAssetDependenciesInfo AssetItemDependency;
//			UFlibAssetManageHelper::GetAssetDependencies(AssetRelativePathItem, AssetItemDependency);
//			resault = UFlibAssetManageHelper::CombineAssetDependencies(resault, AssetItemDependency);
//		}
//		OutDiffDependencies = resault;
//	}
//	return bRunState;
//
//}
//
//void UFlibPatchParserHelper::FilterContentPathInArray(const TArray<FString>& InAllModuleDirList, const TArray<FString>& InAbsPathList, TArray<FString>& OutPathList)
//{
//	for (const auto& AbsPathItem : InAbsPathList)
//	{
//		if (UFlibPatchParserHelper::IsContentAssetPath(InAllModuleDirList, AbsPathItem))
//		{
//			OutPathList.Add(AbsPathItem);
//		}
//	}
//}
//
//bool UFlibPatchParserHelper::IsContentAssetPath(const TArray<FString>& InALLModuleDir, const FString& InAssetAbsPath)
//{
//	FString local_InPath = UFlibAssetManageHelper::ConvPath_BackSlash2Slash(InAssetAbsPath);
//	bool bFileValid = FPaths::FileExists(local_InPath);
//	if ( !bFileValid && !local_InPath.Contains(TEXT("Content/")))
//		return false;
//	
//	for (const auto& ModuleItem : InALLModuleDir)
//	{
//		if (local_InPath.Contains(FPaths::Combine(ModuleItem,TEXT("Content/"))))
//		{
//			return true;
//		}
//	}
//
//	return false;
//}
//
//void UFlibPatchParserHelper::GetAllEnabledModuleBaseDir(TArray<FString>& OutResault)
//{
//	OutResault.Empty();
//
//	TArray<TSharedRef<IPlugin>> AllPlugin = IPluginManager::Get().GetEnabledPlugins();
//
//	for (const auto& PluginItem : AllPlugin)
//	{
//		OutResault.Add(FPaths::ConvertRelativePathToFull(PluginItem.Get().GetBaseDir()));
//	}
//
//	OutResault.Add(UKismetSystemLibrary::GetProjectDirectory());
//	OutResault.Add(FPaths::ConvertRelativePathToFull(FPaths::EngineDir()));
//}

//FString UFlibPatchParserHelper::ConvGitTrackFilePathToAbsPath(const FString& InRepoRootDir, const FString& InTrackFile)
//{
//	FString result;
//	if (!FPaths::DirectoryExists(InRepoRootDir))
//		return result;
//	result = FPaths::Combine(InRepoRootDir, InTrackFile);
//	if (FPaths::FileExists(result))
//		return result;
//	else
//		return TEXT("");
//}
//
TArray<FString> UFlibPatchParserHelper::GetAvailableMaps(FString GameName, bool IncludeEngineMaps, bool Sorted)
{
	TArray<FString> Result;
	TArray<FString> EnginemapNames;
	TArray<FString> ProjectMapNames;

	const FString WildCard = FString::Printf(TEXT("*%s"), *FPackageName::GetMapPackageExtension());

	// Scan all Content folder, because not all projects follow Content/Maps convention
	IFileManager::Get().FindFilesRecursive(ProjectMapNames, *FPaths::Combine(*FPaths::RootDir(), *GameName, TEXT("Content")), *WildCard, true, false);

	// didn't find any, let's check the base GameName just in case it is a full path
	if (ProjectMapNames.Num() == 0)
	{
		IFileManager::Get().FindFilesRecursive(ProjectMapNames, *FPaths::Combine(*GameName, TEXT("Content")), *WildCard, true, false);
	}

	for (int32 i = 0; i < ProjectMapNames.Num(); i++)
	{
		Result.Add(FPaths::GetBaseFilename(ProjectMapNames[i]));
	}

	if (IncludeEngineMaps)
	{
		IFileManager::Get().FindFilesRecursive(EnginemapNames, *FPaths::Combine(*FPaths::RootDir(), TEXT("Engine"), TEXT("Content"), TEXT("Maps")), *WildCard, true, false);

		for (int32 i = 0; i < EnginemapNames.Num(); i++)
		{
			Result.Add(FPaths::GetBaseFilename(EnginemapNames[i]));
		}
	}

	if (Sorted)
	{
		Result.Sort();
	}

	return Result;
}