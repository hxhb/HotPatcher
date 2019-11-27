// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FLibAssetManageHelperEx.generated.h"

USTRUCT(BlueprintType)
struct FPackageInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString AssetName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString AssetGuid;
};

UCLASS()
class ASSETMANAGEREX_API UFLibAssetManageHelperEx : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static FString ConvVirtualToAbsPath(const FString& InPackagePath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool ConvAbsToVirtualPath(const FString& InAbsPath,FString& OutPackagePath);

	// - AssetPath : /Game/BP/BP_Actor.BP_Actor
	// - LongPackageName : /Game/BP/BP_Actor
	// - AssetName : BP_Actor

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static FString GetLongPackageNameFromPackagePath(const FString& InPackagePath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static FString GetAssetNameFromPackagePath(const FString& InPackagePath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool ConvLongPackageNameToPackagePath(const FString& InLongPackageName,FString& OutPackagePath);

		static const FAssetPackageData* GetPackageDataByPackagePath(const FString& InShortPackagePath);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerExEx")
		static bool GetAssetPackageGUID(const FString& InPackagePath, FString& OutGUID);

	// Combine AssetDependencies Filter repeat asset
	UFUNCTION(BlueprintPure, Category = "GWorld|Flib|AssetManager", meta = (CommutativeAssociativeBinaryOperator = "true"))
		static FAssetDependenciesInfo CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B);

	/*
	 @Param InLongPackageName: e.g /Game/TEST/BP_Actor don't pass /Game/TEST/BP_Actor.BP_Actor
	 @Param OutDependices: Output Asset Dependencies
	*/
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependencies(const FString& InLongPackageName, FAssetDependenciesInfo& OutDependices);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetListDependencies(const TArray<FString>& InLongPackageNameList, FAssetDependenciesInfo& OutDependices);

	// recursive scan assets
	static void GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		const FString& InTargetLongPackageName,
		FAssetDependenciesInfo& OutDependencies
	);


	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetModuleAssetsList(const FString& InModuleName, TArray<FString>& OutAssetList);
		
		static bool GetModuleAssets(const FString& InModuleName, TArray<FAssetData>& OutAssetData);
	/*
		TOOLs Set
	*/

	// serialize asset dependencies to json string
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool SerializeAssetDependenciesToJson(const FAssetDependenciesInfo& InAssetDependencies, FString& OutJsonStr);

	// deserialize asset dependencies to FAssetDependenciesIndo from string.
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool DeserializeAssetDependencies(const FString& InStream, FAssetDependenciesInfo& OutAssetDependencies);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool SaveStringToFile(const FString& InFile, const FString& InString);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool LoadFileToString(const FString& InFile, FString& OutString);


	static FString GetAssetBelongModuleName(const FString& InAssetRelativePath);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static void GetAllEnabledModuleName(TArray<FString>& OutEnabledModule);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool GetEnableModuleAbsDir(const FString& InModuleName, FString& OutPath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool GetPluginModuleAbsDir(const FString& InPluginModuleName, FString& OutPath);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool IsValidPlatform(const FString& PlatformName);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static TArray<FString> GetAllTargetPlatform();

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool FindFilesRecursive(const FString& InStartDir, TArray<FString>& OutFileList, bool InRecursive = true);

	// conversion / to \ 
	static FString ConvPath_Slash2BackSlash(const FString& InPath);
	// conversion \ to /
	static FString ConvPath_BackSlash2Slash(const FString& InPath);
};
