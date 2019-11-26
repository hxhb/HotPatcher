// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AssetManager/FAssetDependenciesInfo.h"
#include "AssetManager/FAssetDependenciesDetail.h"

#include "GenericPlatform/GenericPlatformFramePacer.h"
#include "AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibAssetManageHelper.generated.h"

#define JSON_MODULE_LIST_SECTION_NAME TEXT("ModuleList")
#define JSON_ALL_INVALID_ASSET_SECTION_NAME TEXT("InValidAsset")

class FFillArrayDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
{
public:
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (bIsDirectory)
		{
			Directories.Add(FilenameOrDirectory);
		}
		else
		{
			Files.Add(FilenameOrDirectory);
		}
		return true;
	}

	TArray<FString> Directories;
	TArray<FString> Files;
};



UCLASS()
class ASSETMANAGEREX_API UFlibAssetManageHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// from relative asset ref to absolute path
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependencies(const FString& InAssetRelativePath, FAssetDependenciesInfo& OutDependices);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetListDependencies(const TArray<FString>& InAssetRelativePathList, FAssetDependenciesInfo& OutDependices);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependenciesBySoftRef(TSoftObjectPtr<UObject> InAssetRef, FAssetDependenciesInfo& OutDependicesInfo);
	
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static TArray<FString> GetAssetAllDependencies(const FString& InAssetRelativePath);

	// recursive scan assets
	static void GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		const FString& InTargetLongPackageName,
		FAssetDependenciesInfo& OutDependencies
	);

	// conversion relative asset path to absolute path
	// e.g /Game/Maps/GameMap.GameMap to D:\GWorldSlg\Content\Maps\GameMap.umap

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static FString ConvAssetPathFromRelativeToAbs(const FString& InProjectDir, const FString& InRelativePath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void ConvAssetPathListFromRelativeToAbs(const FString& InProjectDir, const TArray<FString>& InRelativePathList, TArray<FString>& OutAbsPath);

	// conversion absolute asset path to relative path
	// e.g D:\GWorldSlg\Content\Maps\GameMap.umap to /Game/Maps/GameMap.GameMap
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static FString ConvAssetPathFromAbsToRelative(const FString& InProjectDir, const FString& InAbsPath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void ConvAssetPathListFromAbsToRelative(const FString& InProjectDir, const TArray<FString>& InAbsPathList,TArray<FString>& OutRelativePath);

	// get ALL asset absolute path from a relative path folder
	// e.g pass /Game return all asset absolute path in the Project
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetAssetAbsPathListFormRelativeFolder(const FString& InProjectDir,const FString& InAssetPath, TArray<FString>& OutAssetList);

	// conversion / to \ 
	static FString ConvPath_Slash2BackSlash(const FString& InPath);
	// conversion \ to /
	static FString ConvPath_BackSlash2Slash(const FString& InPath);
	static bool FindFilesRecursive(const FString& InStartDir, TArray<FString>& OutFileList, bool InRecursive = true, bool InClearList=false);


	// export all asset dependencies in game
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void ExportProjectAssetDependencies(const FString& InProjectDir, FAssetDependenciesInfo& OutAllDependencies);

	// Get all invalid asset list
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAllInValidAssetInProject(const FString& InProjectDir, FAssetDependenciesInfo InAllDependencies, TArray<FString> &OutInValidAsset);

	// serialize asset dependencies to json string
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool SerializeAssetDependenciesToJson(const FAssetDependenciesInfo& InAssetDependencies,FString& OutJsonStr);

	// deserialize asset dependencies to FAssetDependenciesIndo from string.
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool DeserializeAssetDependencies(const FString& InStream,FAssetDependenciesInfo& OutAssetDependencies);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool SaveStringToFile(const FString& InFile, const FString& InString);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool LoadFileToString(const FString& InFile, FString& OutString);


	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetPluginModuleAbsDir(const FString& InPluginModuleName, FString& OutPath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetEnableModuleAbsDir(const FString& InModuleName, FString& OutPath);

	UFUNCTION(BlueprintPure, Category = "GWorld|Flib|AssetManager", meta = (CommutativeAssociativeBinaryOperator = "true"))
	static FAssetDependenciesInfo CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static FAssetDependenciesInfo ParserNewDependencysInfo(const FAssetDependenciesInfo& InNewVersion, const FAssetDependenciesInfo& InOldVersion);


	// 将游戏内的资源路径转换为Cooked之后的资源路径
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool ConvAssetRelativePathToCookedPath(const FString& InProjectAbsDir,const FString& InPlatformName, const FString& InRelativeAssetPath, TArray<FString>& OutCookedAssetPath, TArray<FString>& OutCookedAssetRelativePath);

	static bool IsValidPlatform(const FString& PlatformName);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetCookCommandFromAssetDependencies(const FString& InProjectDir, const FString& InPlatformName, const FAssetDependenciesInfo& InAssetDependencies, const TArray<FString> &InCookParams, TArray<FString>& OutCookCommand);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool CombineCookedAssetCommand(const TArray<FString> &InAbsPath, const TArray<FString>& InRelativePath, const TArray<FString>& InParams, TArray<FString>& OutCommand);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool ExportCookPakCommandToFile(const TArray<FString>& InCommand, const FString& InFile);
};
