// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AssetManager/FAssetDependenciesInfo.h"
#include "AssetManager/FAssetDetail.h"
#include "AssetRegistry.h"
// engine
#include "Engine/StreamableManager.h"
#include "Engine/EngineTypes.h"
#include "Dom/JsonValue.h"
#include "Templates/SharedPointer.h"
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibAssetManageHelper.generated.h"

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

UENUM(BlueprintType)
enum class EAssetRegistryDependencyTypeEx :uint8
{
	None = 0x00,
	// Dependencies which don't need to be loaded for the object to be used (i.e. soft object paths)
	Soft = 0x01,

	// Dependencies which are required for correct usage of the source asset, and must be loaded at the same time
	Hard = 0x02,

	// References to specific SearchableNames inside a package
	SearchableName = 0x04,

	// Indirect management references, these are set through recursion for Primary Assets that manage packages or other primary assets
	SoftManage = 0x08,

	// Reference that says one object directly manages another object, set when Primary Assets manage things explicitly
	HardManage = 0x10,

	Packages = Soft | Hard,

	Manage = SoftManage | HardManage,

	All = Soft | Hard | SearchableName | SoftManage | HardManage
};

UENUM(BlueprintType)
enum class EHotPatcherMatchModEx :uint8
{
	StartWith,
	Equal
};
UCLASS()
class HOTPATCHERRUNTIME_API UFlibAssetManageHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static FString PackagePathToFilename(const FString& InPackagePath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static FString LongPackageNameToFilename(const FString& InLongPackageName);
	
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
		static bool FilenameToPackagePath(const FString& InAbsPath,FString& OutPackagePath);


	static void UpdateAssetMangerDatabase(bool bForceRefresh);
	// - AssetPath : /Game/BP/BP_Actor.BP_Actor
	// - LongPackageName : /Game/BP/BP_Actor
	// - AssetName : BP_Actor

	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManagerEx")
	static FString LongPackageNameToPackagePath(const FString& InLongPackageName);
	static FString PackagePathToLongPackageName(const FString& PackagePath);
	
	static FAssetPackageData* GetPackageDataByPackageName(const FString& InPackageName);

	static bool GetAssetPackageGUID(const FString& InPackageName, FName& OutGUID);
	static bool GetAssetPackageGUID(FAssetDetail& AssetDetail);
	static bool GetWPWorldGUID(FAssetDetail& AssetDetail);

	static FSoftObjectPath CreateSoftObjectPathByPackage(UPackage* Package);
	static FName GetAssetTypeByPackage(UPackage* Package);
	static FName GetAssetType(FSoftObjectPath InPackageName);
	
	// Combine AssetDependencies Filter repeat asset
	UFUNCTION(BlueprintPure, Category = "GWorld|Flib|AssetManager", meta = (CommutativeAssociativeBinaryOperator = "true"))
		static FAssetDependenciesInfo CombineAssetDependencies(const FAssetDependenciesInfo& A, const FAssetDependenciesInfo& B);

	// Get All invalid reference asset
	static void GetAllInValidAssetInProject(FAssetDependenciesInfo InAllDependencies, TArray<FString> &OutInValidAsset, TArray<FString> InIgnoreModules = {});


	static bool GetAssetReferenceByLongPackageName(const FString& LongPackageName,const TArray<EAssetRegistryDependencyType::Type>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset);
	static bool GetAssetReference(const FAssetDetail& InAsset,const TArray<EAssetRegistryDependencyType::Type>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset);
	static void GetAssetReferenceRecursively(const FAssetDetail& InAsset,
	                                         const TArray<EAssetRegistryDependencyType::Type>& SearchAssetDepTypes,
	                                         const TArray<FString>& SearchAssetsTypes,
	                                         TArray<FAssetDetail>& OutRefAsset, bool bRecursive = true);
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetAssetReferenceEx(const FAssetDetail& InAsset, const TArray<EAssetRegistryDependencyTypeEx>& SearchAssetDepTypes, TArray<FAssetDetail>& OutRefAsset);
	

	static FAssetDetail GetAssetDetailByPackageName(const FString& InPackageName);
	
	
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool GetRedirectorList(const TArray<FString>& InFilterPackagePaths, TArray<FAssetDetail>& OutRedirector);
		static bool GetSpecifyAssetData(const FString& InLongPackageName, TArray<FAssetData>& OutAssetData,bool InIncludeOnlyOnDiskAssets);
		// /Game all uasset/umap files
		static TArray<FString> GetAssetsByFilter(const FString& InFilter);
		static bool GetAssetsData(const TArray<FString>& InFilterPaths, TArray<FAssetData>& OutAssetData, bool bLocalIncludeOnlyOnDiskAssets = true);
		static bool GetAssetsDataByDisk(const TArray<FString>& InFilterPaths, TArray<FAssetData>& OutAssetData);
		static bool GetSingleAssetsData(const FString& InPackagePath, FAssetData& OutAssetData);
		static bool GetAssetsDataByPackageName(const FString& InPackageName, FAssetData& OutAssetData);
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

	static FString GetCookedPathByLongPackageName(	const FString& InProjectAbsDir,const FString& InPlatformName,const FString& InLongPackageName,const FString& OverrideCookedDir);
	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool ConvLongPackageNameToCookedPath(
			const FString& InProjectAbsDir,
			const FString& InPlatformName,
			const FString& InLongPackageName,
			TArray<FString>& OutCookedAssetPath,
			TArray<FString>& OutCookedAssetRelativePath,
			const FString& OverrideCookedDir,
			FCriticalSection& LocalSynchronizationObject
			);
	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool MakePakCommandFromAssetDependencies(
			const FString& InProjectDir,
			const FString& OverrideCookedDir,
			const FString& InPlatformName,
			const FAssetDependenciesInfo& InAssetDependencies,
			TFunction<void(const TArray<FString>&,const TArray<FString>&,const FString&,const FString&,FCriticalSection&)> InReceivePakCommand = [](const TArray<FString>&,const TArray<FString>&, const FString&, const FString&,FCriticalSection&) {},
			TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset = [](const FString&)->bool{return false;}
		);
	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool MakePakCommandFromLongPackageName(
			const FString& InProjectDir,
			const FString& OverrideCookedDir,
			const FString& InPlatformName,
			const FString& InAssetLongPackageName,
			FCriticalSection& LocalSynchronizationObject,
			TFunction<void(const TArray<FString>&, const TArray<FString>&,const FString&, const FString&,FCriticalSection&)>  InReceivePakCommand = [](const TArray<FString>&,const TArray<FString>&, const FString&, const FString&,FCriticalSection&) {},
			TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset = [](const FString&)->bool{return false;}
		);
	// UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool CombineCookedAssetCommand(
			const TArray<FString> &InAbsPath,
			const TArray<FString>& InRelativePath,
			TArray<FString>& OutPakCommand,
			TArray<FString>& OutIoStoreCommand,
			TFunction<bool(const FString& CookedAssetsAbsPath)> IsIoStoreAsset = [](const FString&)->bool{return false;}
		);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static bool ExportCookPakCommandToFile(const TArray<FString>& InCommand, const FString& InFile);

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
		static bool FindFilesRecursive(const FString& InStartDir, TArray<FString>& OutFileList, bool InRecursive = true);

	static uint32 ParserAssetDependenciesInfoNumber(const FAssetDependenciesInfo& AssetDependenciesInfo, TMap<FString,uint32>);
	static FString ParserModuleAssetsNumMap(const TMap<FString,uint32>& InMap);

	static EAssetRegistryDependencyType::Type ConvAssetRegistryDependencyToInternal(const EAssetRegistryDependencyTypeEx& InType);

	static void GetAssetDataInPaths(const TArray<FString>& Paths, TArray<FAssetData>& OutAssetData);
	
	static void ExcludeContentForAssetDependenciesDetail(FAssetDependenciesInfo& AssetDependencies,const TArray<FString>& ExcludeRules = {TEXT("")},EHotPatcherMatchModEx MatchMod = EHotPatcherMatchModEx::StartWith);
	
	static TArray<FString> DirectoriesToStrings(const TArray<FDirectoryPath>& DirectoryPaths);
	static TSet<FString> SoftObjectPathsToStringsSet(const TArray<FSoftObjectPath>& SoftObjectPaths);
	static TArray<FString> SoftObjectPathsToStrings(const TArray<FSoftObjectPath>& SoftObjectPaths);
	static TSet<FName> GetClassesNames(const TArray<UClass*> CLasses);
	
	static FString NormalizeContentDir(const FString& Dir);
	static TArray<FString> NormalizeContentDirs(const TArray<FString>& Dirs);
	
	static FStreamableManager& GetStreamableManager();

	// Default priority for all async loads
	static const uint32 DefaultAsyncLoadPriority = 0;
	// Priority to try and load immediately
	static const uint32 AsyncLoadHighPriority = 100;
	static TSharedPtr<FStreamableHandle> LoadObjectAsync(FSoftObjectPath ObjectPath,TFunction<void(FSoftObjectPath)> Callback,uint32 Priority);
	static void LoadPackageAsync(FSoftObjectPath ObjectPath,TFunction<void(FSoftObjectPath,bool)> Callback = nullptr,uint32 Priority = DefaultAsyncLoadPriority);
	static UPackage* LoadPackage( UPackage* InOuter, const TCHAR* InLongPackageName, uint32 LoadFlags, FArchive* InReaderOverride = nullptr);
	static UPackage* GetPackage(FName PackageName);

	// static TArray<UPackage*> GetPackagesByClass(TArray<UPackage*>& Packages, UClass* Class, bool RemoveFromSrc);
	
	static TArray<UPackage*> LoadPackagesForCooking(const TArray<FSoftObjectPath>& SoftObjectPaths, bool bStorageConcurrent);
	
	static bool MatchIgnoreTypes(const FString& LongPackageName, TSet<FName> IgnoreTypes, FString& MatchTypeStr);
	static bool MatchIgnoreFilters(const FString& LongPackageName, const TArray<FString>& IgnoreDirs, FString& MatchDir);

	static bool ContainsRedirector(const FName& PackageName, TMap<FName, FName>& RedirectedPaths);

	static TArray<UObject*> FindClassObjectInPackage(UPackage* Package,UClass* FindClass);
	static bool HasClassObjInPackage(UPackage* Package,UClass* FindClass);
	static TArray<FAssetDetail> GetAssetDetailsByClass(TArray<FAssetDetail>& AllAssetDetails,UClass* Class,bool RemoveFromSrc);
	static TArray<FSoftObjectPath> GetAssetPathsByClass(TArray<FAssetDetail>& AllAssetDetails,UClass* Class,bool RemoveFromSrc);

	static bool IsRedirector(const FAssetDetail& Src,FAssetDetail& Out);
	static void ReplaceReditector(TArray<FAssetDetail>& SrcAssets);
	static void RemoveInvalidAssets(TArray<FAssetDetail>& SrcAssets);

	static FName GetAssetDataClasses(const FAssetData& Data);
	static FName GetObjectPathByAssetData(const FAssetData& Data);
	static bool bIncludeOnlyOnDiskAssets;

	static void UpdateAssetRegistryData(const FString& PackageName);
	static TArray<FString> GetPackgeFiles(const FString& LongPackageName,const FString& Extension);

	static FString GetAssetPath(const FSoftObjectPath& ObjectPath);
	static FAssetData GetAssetByObjectPath(FName Path);

	static FString GetBaseFilename(const FString& InPath,ESearchDir::Type SearchMode,bool bRemovePath = true);

	static bool GenerateMD5(const FString& Filename,FName& OutGUID);
	static bool GenerateMD5(const FString& Filename,FString& OutGUID);
};


