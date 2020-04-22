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


	static void UpdateAssetMangerDatabase(bool bForceRefresh);
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
	static void GetAllInValidAssetInProject(FAssetDependenciesInfo InAllDependencies, TArray<FString> &OutInValidAsset, TArray<FString> InIgnoreModules = {});

	/*
	 @Param InLongPackageName: e.g /Game/TEST/BP_Actor don't pass /Game/TEST/BP_Actor.BP_Actor
	 @Param OutDependices: Output Asset Dependencies
	*/
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependencies(const FString& InLongPackageName, FAssetDependenciesInfo& OutDependices);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetListDependencies(const TArray<FString>& InLongPackageNameList, FAssetDependenciesInfo& OutDependices);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetAssetDependency(const FString& InLongPackageName, TArray<FAssetDetail>& OutRefAsset, bool bRecursively = true);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetAssetDependencyByDetail(const FAssetDetail& InAsset, TArray<FAssetDetail>& OutRefAsset, bool bRecursively = true);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetAssetReference(const FAssetDetail& InAsset, TArray<FAssetDetail>& OutRefAsset);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependenciesForAssetDetail(const FAssetDetail& InAssetDetail , FAssetDependenciesInfo& OutDependices);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetListDependenciesForAssetDetail(const TArray<FAssetDetail>& InAssetsDetailList, FAssetDependenciesInfo& OutDependices);

	// 获取FAssetDependenciesInfo中所有的FAssetDetail
	static void GetAssetDetailsByAssetDependenciesInfo(const FAssetDependenciesInfo& InAssetDependencies,TArray<FAssetDetail>& OutAssetDetails);

	static TArray<FString> GetAssetLongPackageNameByAssetDependenciesInfo(const FAssetDependenciesInfo& InAssetDependencies);

	// recursive scan assets
	static void GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		const FString& InTargetLongPackageName,
		FAssetDependenciesInfo& OutDependencies,
		bool bRecursively=true
	);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager",meta=(AutoCreateRefTerm="InExFilterPackagePaths",AdvancedDisplay="InExFilterPackagePaths"))
		static bool GetModuleAssetsList(const FString& InModuleName,const TArray<FString>& InExFilterPackagePaths, TArray<FAssetDetail>& OutAssetList);

	/*
	 * FilterPackageName format is /Game or /Game/TEST
	 */
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetAssetsList(const TArray<FString>& InFilterPackagePaths,TArray<FAssetDetail>& OutAssetList, bool bReTargetRedirector=true);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetRedirectorList(const TArray<FString>& InFilterPackagePaths, TArray<FAssetDetail>& OutRedirector);
		static bool GetSpecifyAssetData(const FString& InLongPackageName, TArray<FAssetData>& OutAssetData,bool InIncludeOnlyOnDiskAssets);
		static bool GetAssetsData(const TArray<FString>& InFilterPackagePaths, TArray<FAssetData>& OutAssetData);
		static bool GetSingleAssetsData(const FString& InPackagePath, FAssetData& OutAssetData);
		static bool GetClassStringFromFAssetData(const FAssetData& InAssetData,FString& OutAssetType);
		static bool ConvFAssetDataToFAssetDetail(const FAssetData& InAssetData,FAssetDetail& OutAssetDetail);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetSpecifyAssetDetail(const FString& InLongPackageName, FAssetDetail& OutAssetDetail);
	
	// 过滤掉没有引用的资源
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void FilterNoRefAssets(const TArray<FAssetDetail>& InAssetsDetail, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void FilterNoRefAssetsWithIgnoreFilter(const TArray<FAssetDetail>& InAssetsDetail,const TArray<FString>& InIgnoreFilters, TArray<FAssetDetail>& OutHasRefAssetsDetail, TArray<FAssetDetail>& OutDontHasRefAssetsDetail);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool CombineAssetsDetailAsFAssetDepenInfo(const TArray<FAssetDetail>& InAssetsDetailList,FAssetDependenciesInfo& OutAssetInfo);

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool ConvLongPackageNameToCookedPath(const FString& InProjectAbsDir, const FString& InPlatformName, const FString& InLongPackageName, TArray<FString>& OutCookedAssetPath, TArray<FString>& OutCookedAssetRelativePath);
	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool MakePakCommandFromAssetDependencies(const FString& InProjectDir, const FString& InPlatformName, const FAssetDependenciesInfo& InAssetDependencies, const TArray<FString> &InCookParams, TArray<FString>& OutCookCommand, TFunction<void(const TArray<FString>&,const FString&,const FString&)> InReceivePakCommand = [](const TArray<FString>&, const FString&, const FString&) {});
	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool MakePakCommandFromLongPackageName(const FString& InProjectDir, const FString& InPlatformName, const FString& InAssetLongPackageName, const TArray<FString> &InCookParams, TArray<FString>& OutCookCommand, TFunction<void(const TArray<FString>&, const FString&, const FString&)>  InReceivePakCommand = [](const TArray<FString>&, const FString&, const FString&) {});
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
	static bool SerializeAssetDependenciesToJsonObject(const FAssetDependenciesInfo& InAssetDependencies, TSharedPtr<FJsonObject>& OutJsonObject, TArray<FString> InIgnoreModules={});
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

	// conv /Game/AAAA/ to /D:/PROJECTNAME/Content/AAAA/
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool ConvRelativeDirToAbsDir(const FString& InRelativePath, FString& OutAbsPath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static void GetAllEnabledModuleName(TMap<FString, FString>& OutModules);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool GetModuleNameByRelativePath(const FString& InRelativePath, FString& OutModuleName);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool ModuleIsEnabled(const FString& InModuleName);
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

	// conversion slash to back slash
	static FString ConvPath_Slash2BackSlash(const FString& InPath);
	// conversion back slash to slash
	static FString ConvPath_BackSlash2Slash(const FString& InPath);
};
