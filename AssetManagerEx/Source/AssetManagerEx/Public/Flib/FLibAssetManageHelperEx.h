// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AssetManager/FAssetDependenciesInfo.h"
#include "AssetManager/FAssetDetail.h"

#include "Templates/SharedPointer.h"
#include "AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FLibAssetManageHelperEx.generated.h"


#define JSON_MODULE_LIST_SECTION_NAME TEXT("ModuleList")
#define JSON_ALL_INVALID_ASSET_SECTION_NAME TEXT("InValidAsset")
#define JSON_ALL_ASSETS_LIST_SECTION_NAME TEXT("AssetsList")
#define JSON_ALL_ASSETS_Detail_SECTION_NAME TEXT("AssetsDetail")

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
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool ConvPackagePathToLongPackageName(const FString& InPackagePath, FString& OutLongPackageName);

		static const FAssetPackageData* GetPackageDataByPackagePath(const FString& InShortPackagePath);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerExEx")
		static bool GetAssetPackageGUID(const FString& InPackagePath, FString& OutGUID);

	// Combine AssetDependencies Filter repeat asset
	UFUNCTION(BlueprintPure, Category = "GWorld|Flib|AssetManager", meta = (CommutativeAssociativeBinaryOperator = "true"))
		static FAssetDependenciesInfo CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B);

	// Get All invalid reference asset
	static void GetAllInValidAssetInProject(FAssetDependenciesInfo InAllDependencies, TArray<FString> &OutInValidAsset);

	/*
	 @Param InLongPackageName: e.g /Game/TEST/BP_Actor don't pass /Game/TEST/BP_Actor.BP_Actor
	 @Param OutDependices: Output Asset Dependencies
	*/
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependencies(const FString& InLongPackageName, FAssetDependenciesInfo& OutDependices);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetListDependencies(const TArray<FString>& InLongPackageNameList, FAssetDependenciesInfo& OutDependices);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependenciesForAssetDetail(const FAssetDetail& InAssetDetail , FAssetDependenciesInfo& OutDependices);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetListDependenciesForAssetDetail(const TArray<FAssetDetail>& InAssetsDetailList, FAssetDependenciesInfo& OutDependices);


	// recursive scan assets
	static void GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		const FString& InTargetLongPackageName,
		FAssetDependenciesInfo& OutDependencies
	);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager",meta=(AutoCreateRefTerm="InExFilterPackagePaths",AdvancedDisplay="InExFilterPackagePaths"))
		static bool GetModuleAssetsList(const FString& InModuleName,const TArray<FString>& InExFilterPackagePaths, TArray<FAssetDetail>& OutAssetList);

	/*
	 * FilterPackageName format is /Game or /Game/TEST
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetAssetsList(const TArray<FString>& InFilterPackagePaths,TArray<FAssetDetail>& OutAssetList);

		static bool GetAssetsData(const TArray<FString>& InFilterPackagePaths, TArray<FAssetData>& OutAssetData);
		static bool GetSingleAssetsData(const FString& InPackagePath, FAssetData& OutAssetData);
		static bool GetClassStringFromFAssetData(const FAssetData& InAssetData,FString& OutAssetType);
	
	// 过滤掉没有引用的资源
		UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
			static void FilterNoRefAssets(const TArray<FAssetDetail>& InAssetsDetail, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool CombineAssetsDetailAsFAssetDepenInfo(const TArray<FAssetDetail>& InAssetsDetailList,FAssetDependenciesInfo& OutAssetInfo);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool ConvLongPackageNameToCookedPath(const FString& InProjectAbsDir, const FString& InPlatformName, const FString& InLongPackageName, TArray<FString>& OutCookedAssetPath, TArray<FString>& OutCookedAssetRelativePath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetCookCommandFromAssetDependencies(const FString& InProjectDir, const FString& InPlatformName, const FAssetDependenciesInfo& InAssetDependencies, const TArray<FString> &InCookParams, TArray<FString>& OutCookCommand);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool CombineCookedAssetCommand(const TArray<FString> &InAbsPath, const TArray<FString>& InRelativePath, const TArray<FString>& InParams, TArray<FString>& OutCommand);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool ExportCookPakCommandToFile(const TArray<FString>& InCommand, const FString& InFile);

	/*
		TOOLs Set
	*/

	// serialize asset dependencies to json string
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool SerializeAssetDependenciesToJson(const FAssetDependenciesInfo& InAssetDependencies, FString& OutJsonStr);
	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool SerializeAssetDependenciesToJsonObject(const FAssetDependenciesInfo& InAssetDependencies, TSharedPtr<FJsonObject>& OutJsonObject);
	// deserialize asset dependencies to FAssetDependenciesIndo from string.
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool DeserializeAssetDependencies(const FString& InStream, FAssetDependenciesInfo& OutAssetDependencies);

		static bool DeserializeAssetDependenciesForJsonObject(const TSharedPtr<FJsonObject>& InJsonObject, FAssetDependenciesInfo& OutAssetDependencies);

	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static TSharedPtr<FJsonObject> SerilizeAssetDetial(const FAssetDetail& InAssetDetail);
		static bool DeserilizeAssetDetial(TSharedPtr<FJsonObject>& InJsonObject,FAssetDetail& OutAssetDetail);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static FString SerializeAssetDetialArrayToString(const TArray<FAssetDetail>& InAssetDetialList);

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
