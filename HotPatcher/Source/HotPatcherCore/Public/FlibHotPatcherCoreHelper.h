// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BaseTypes/FExternFileInfo.h"
#include "BaseTypes/FExternDirectoryInfo.h"
#include "BaseTypes/FPatcherSpecifyAsset.h"
#include "BaseTypes/HotPatcherBaseTypes.h"
#include "BaseTypes/FHotPatcherVersion.h"
#include "BaseTypes/FChunkInfo.h"
#include "BaseTypes/ETargetPlatform.h"
#include "CreatePatch/FExportPatchSettings.h"

// engine header
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#if WITH_PACKAGE_CONTEXT
	#include "UObject/SavePackage.h"
	#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 25
	 #include "Serialization/BulkDataManifest.h"
	#endif
#endif

#include "FlibHotPatcherCoreHelper.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherCoreHelper, Log, All);

struct FExportPatchSettings;



struct FExecTimeRecoder
{
	FExecTimeRecoder(const FString& InDispalyStr):DispalyStr(InDispalyStr)
	{
		BeginTime = FDateTime::Now();
	}
	~FExecTimeRecoder()
	{
		EndTime = FDateTime::Now();
		FTimespan ClusterExecTime = EndTime - BeginTime;
		UE_LOG(LogHotPatcherCoreHelper,Display,TEXT("%s Time : %fs"),*DispalyStr,ClusterExecTime.GetTotalSeconds())
	}
	FString DispalyStr;
	FDateTime BeginTime;
	FDateTime EndTime;
};

struct FProjectPackageAssetCollection
{
	TArray<FDirectoryPath> DirectoryPaths;
	TArray<FDirectoryPath> NeverCookPaths;
	TArray<FSoftObjectPath> NeedCookPackages;
	TArray<FSoftObjectPath> NeverCookPackages;
};

/**
 * 
 */
UCLASS()
class HOTPATCHERCORE_API UFlibHotPatcherCoreHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "HotPatcherCore")
		static TArray<FString> GetAllCookOption();

	static void CheckInvalidCookFilesByAssetDependenciesInfo(
		const FString& InProjectAbsDir,
		const FString& OverrideCookedDir,
		const FString& InPlatformName,
		const FAssetDependenciesInfo& InAssetDependencies,
		TArray<FAssetDetail>& OutValidAssets,
		TArray<FAssetDetail>& OutInvalidAssets);

	static FChunkInfo MakeChunkFromPatchSettings(struct FExportPatchSettings const* InPatchSetting);
	static FChunkInfo MakeChunkFromPatchVerison(const FHotPatcherVersion& InPatchVersion);
	static FString GetAssetCookedSavePath(const FString& BaseDir, const FString PacakgeName, const FString& Platform,bool bSkipPlatform = false);

	static FString GetProjectCookedDir();
#if WITH_PACKAGE_CONTEXT
	static FSavePackageContext* CreateSaveContext(const ITargetPlatform* TargetPlatform,bool bUseZenLoader,const FString& OverrideCookedDir);
	static TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> CreatePlatformsPackageContexts(const TArray<ETargetPlatform>& Platforms,bool bIoStore,const FString& OverrideCookedDir);
	static bool SavePlatformBulkDataManifest(TMap<ETargetPlatform, TSharedPtr<FSavePackageContext>>&PlatformSavePackageContexts,ETargetPlatform Platform);
#endif

	static bool CookPackages(
		const TArray<UPackage*> Packages,
		TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms,
		FCookActionCallback CookActionCallback,
	#if WITH_PACKAGE_CONTEXT
		class TMap<FString,FSavePackageContext*> PlatformSavePackageContext,
	#endif
		const TMap<FName,FString>& CookedPlatformSavePaths,
		bool bStorageConcurrent
	);
	
	static bool CookPackage(
		UPackage* Package,
		TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms,
		FCookActionCallback CookActionCallback,
#if WITH_PACKAGE_CONTEXT
		class TMap<FString,FSavePackageContext*> PlatformSavePackageContext,
#endif
		const TMap<FName,FString>& CookedPlatformSavePaths,
		bool bStorageConcurrent
	);

	static bool UseCookCmdlet(UPackage* Package,TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms);
	static bool RunCmdlet(const FString& CmdletName,const FString& Params,int32& OutRetValue);
	static bool CookPackagesByCmdlet(
		const TArray<UPackage*> Packages,
		TMap<ETargetPlatform,ITargetPlatform*> CookPlatforms,
		FCookActionCallback CookActionCallback,
		const TMap<FName,FString>& CookedPlatformSavePaths,
		bool bSharedMaterialLibrary = false
	);
	static bool CookByCmdlet(
		const TArray<FString>& LongPackageNames,
		ETargetPlatform TargetPlatform,
		const FString& SaveToCookedDir = TEXT(""),
		bool bSharedMaterialLibrary = false
	);

	static void CookChunkAssets(
		TArray<FAssetDetail> Assets,
		const TArray<ETargetPlatform>& Platforms,
		FCookActionCallback CookActionCallback,
#if WITH_PACKAGE_CONTEXT
		class TMap<ETargetPlatform,FSavePackageContext*> PlatformSavePackageContext = TMap<ETargetPlatform,FSavePackageContext*>{},
#endif
		const FString& InSavePath = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),TEXT("Cooked"))
	);
	
	static ITargetPlatform* GetTargetPlatformByName(const FString& PlatformName);
	static TArray<ITargetPlatform*> GetTargetPlatformsByNames(const TArray<ETargetPlatform>& PlatformNames);

	UFUNCTION(BlueprintCallable, Category = "HotPatcherCore")
    static FString GetUnrealPakBinary();
	UFUNCTION(BlueprintCallable, Category = "HotPatcherCore")
        static FString GetUECmdBinary();

	static FProcHandle DoUnrealPak(TArray<FString> UnrealPakCommandletOptions, bool block);

	static FString GetMetadataDir(const FString& ProjectDir,const FString& ProjectName,ETargetPlatform Platform);
	static void CleanDefaultMetadataCache(const TArray<ETargetPlatform>& TargetPlatforms);
	
	static void BackupMetadataDir(const FString& ProjectDir,const FString& ProjectName,const TArray<ETargetPlatform>& Platforms,const FString& OutDir);
	static void BackupProjectConfigDir(const FString& ProjectDir,const FString& OutDir);

	static FString ReleaseSummary(const FHotPatcherVersion& NewVersion);
	static FString PatchSummary(const FPatchVersionDiff& DiffInfo);

	
	static FString ReplacePakRegular(const FReplacePakRegular& RegularConf, const FString& InRegular);
	static bool CheckSelectedAssetsCookStatus(const FString& OverrideCookedDir,const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg);
	static bool CheckPatchRequire(const FString& OverrideCookedDir,const FPatchVersionDiff& InDiff,const TArray<FString>& PlatformNames,FString& OutMsg);

	// WindowsNoEditor to Windows
	static FString Conv2IniPlatform(const FString& Platform);
	static TArray<FString> GetSupportPlatforms();

	static FString GetEncryptSettingsCommandlineOptions(const FPakEncryptSettings& EncryptSettings,const FString& PlatformName);

	static ITargetPlatform* GetPlatformByName(const FString& Name);

	/*
	* 0x1 Add
	* 0x2 Modyfy
	*/
	static void AnalysisDependenciesAssets(const FHotPatcherVersion& Base, const FHotPatcherVersion& New,FPatchVersionDiff& PakDiff,UClass* SearchClass,TArray<UClass*> DependenciesClass,bool bRecursive,int32 flags = 0x1|0x2);
	static void AnalysisWidgetTree(const FHotPatcherVersion& Base, const FHotPatcherVersion& New,FPatchVersionDiff& PakDiff,int32 flags = 0x1|0x2);
	static void AnalysisMaterialInstance(const FHotPatcherVersion& Base, const FHotPatcherVersion& New,FPatchVersionDiff& PakDiff,int32 flags= 0x1|0x2);
	
	static FPatchVersionDiff DiffPatchVersionWithPatchSetting(const struct FExportPatchSettings& PatchSetting, const FHotPatcherVersion& Base, const FHotPatcherVersion& New);

	// CurrenrVersionChunk中的过滤器会进行依赖分析，TotalChunk的不会，目的是让用户可以自己控制某个文件夹打包到哪个Pak里，而不会对该文件夹下的资源进行依赖分析
	static FChunkAssetDescribe DiffChunkWithPatchSetting(
		const struct FExportPatchSettings& PatchSetting,
		const FChunkInfo& CurrentVersionChunk,
		const FChunkInfo& TotalChunk
		//,TMap<FString, FAssetDependenciesInfo>& ScanedCaches
	);
	static FChunkAssetDescribe DiffChunkByBaseVersionWithPatchSetting(
		const struct FExportPatchSettings& PatchSetting,
		const FChunkInfo& CurrentVersionChunk,
		const FChunkInfo& TotalChunk,
		const FHotPatcherVersion& BaseVersion
		//,TMap<FString, FAssetDependenciesInfo>& ScanedCaches
	);
	static bool SerializeAssetRegistryByDetails(IAssetRegistry* AssetRegistry, const FString& PlatformName, const TArray<FAssetDetail>& AssetDetails, const FString& SavePath, FAssetRegistrySerializationOptions SaveOptions);
	static bool SerializeAssetRegistryByDetails(IAssetRegistry* AssetRegistry, const FString& PlatformName, const TArray<FAssetDetail>& AssetDetails, const FString& SavePath);
	static bool SerializeAssetRegistry(IAssetRegistry* AssetRegistry, const FString& PlatformName, const TArray<FString>& PackagePaths, const FString& SavePath, FAssetRegistrySerializationOptions
	                                   SaveOptions);
	
	static FHotPatcherVersion MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion, FExportPatchSettings* InPatchSettings);
	static FHotPatcherVersion MakeNewReleaseByDiff(const FHotPatcherVersion& InBaseVersion, const FPatchVersionDiff& InDiff, FExportPatchSettings* InPatchSettings = NULL);

public:
	// In: "C:\Users\lipengzha\Documents\UnrealProjects\Blank425\Intermediate\Staging\Blank425.upluginmanifest" "../../../Blank425/Plugins/Blank425.upluginmanifest
	// To: upluginmanifest
	static FString GetPakCommandExtersion(const FString& InCommand);

	// [Pak]
	// +ExtensionsToNotUsePluginCompression=uplugin
	// +ExtensionsToNotUsePluginCompression=upluginmanifest
	// +ExtensionsToNotUsePluginCompression=uproject
	// +ExtensionsToNotUsePluginCompression=ini
	// +ExtensionsToNotUsePluginCompression=icu
	// +ExtensionsToNotUsePluginCompression=res
	static TArray<FString> GetExtensionsToNotUsePluginCompressionByGConfig();

	static void AppendPakCommandOptions(
		TArray<FString>& OriginCommands,
		const TArray<FString>& Options,
		bool bAppendAllMatch,
		const TArray<FString>& AppendFileExtersions,
		const TArray<FString>& IgnoreFormats,
		const TArray<FString>& InIgnoreOptions);

	static FProjectPackageAssetCollection ImportProjectSettingsPackages();
	
	static void WaitDistanceFieldAsyncQueueComplete();
	static void WaitForAsyncFileWrites();
	static void WaitDDCComplete();
	static bool IsCanCookPackage(const FString& LongPackageName);
	
	static void ImportProjectSettingsToScannerConfig(FAssetScanConfig& AssetScanConfig);
	static void ImportProjectNotAssetDir(TArray<FPlatformExternAssets>& PlatformExternAssets, ETargetPlatform AddToTargetPlatform);
	
	static TArray<FExternDirectoryInfo> GetProjectNotAssetDirConfig();
	
	static void CacheForCookedPlatformData(
		const TArray<UPackage*>& Packages,
		TArray<ITargetPlatform*> TargetPlatforms,
		TSet<UObject*>& ProcessedObjs,
		TSet<UObject*>& PendingCachePlatformDataObjects,
		bool bStorageConcurrent = false,
		bool bWaitComplete = false,
		TFunction<void(UPackage*,UObject*)> OnPreCacheObjectWithOuter=nullptr
		);
	static void CacheForCookedPlatformData(
		const TArray<FSoftObjectPath>& ObjectPaths,
		TArray<ITargetPlatform*> TargetPlatforms,
		TSet<UObject*>& ProcessedObjs,
		TSet<UObject*>& PendingCachePlatformDataObjects,
		bool bStorageConcurrent,
		bool bWaitComplete = false,
		TFunction<void(UPackage*,UObject*)> OnPreCacheObjectWithOuter = nullptr
		);

	static void WaitObjectsCachePlatformDataComplete(TSet<UObject*>& CachedPlatformDataObjects,TSet<UObject*>& PendingCachePlatformDataObjects,TArray<ITargetPlatform*> TargetPlatforms);
	
	static uint32 GetCookSaveFlag(UPackage* Package,bool bUnversioned = true,bool bStorageConcurrent = false,bool CookLinkerDiff = false);
	static EObjectFlags GetObjectFlagForCooked(UPackage* Package);
	
	static TArray<FAssetDetail> GetReferenceRecursivelyByClassName(const FAssetDetail& AssetDetail,const TArray<FString>& AssetTypeNames,const TArray<EAssetRegistryDependencyTypeEx>& RefType,bool bRecursive = true);

	static TArray<UClass*> GetDerivedClasses(UClass* BaseClass,bool bRecursive,bool bContainSelf = false);

	static void DeleteDirectory(const FString& Dir);

	static int32 GetMemoryMappingAlignment(const FString& PlatformName);

	static void SaveGlobalShaderMapFiles(const TArrayView<const ITargetPlatform* const>& Platforms,const FString& BaseOutputDir);

	static FString GetSavePackageResultStr(ESavePackageResult Result);

	static void AdaptorOldVersionConfig(FAssetScanConfig& ScanConfig,const FString& JsonContent);
	
	static bool GetIniPlatformName(const FString& PlatformName,FString& OutIniPlatformName);
	
	// need add UNREALED_API to FAssetRegistryGenerator
	// all chunksinfo.csv / pakchunklist.txt / assetregistry.bin
	static bool SerializeChunksManifests(ITargetPlatform* TargetPlatform, const TSet<FName>&, const TSet<FName>&, bool bGenerateStreamingInstallManifest = true);
	static TArray<UClass*> GetClassesByNames(const TArray<FName>& ClassesNames);
	static TArray<UClass*> GetAllMaterialClasses();
	static bool IsMaterialClasses(UClass* Class);
	static bool IsMaterialClassName(FName ClassName);
	static bool AssetDetailsHasClasses(const TArray<FAssetDetail>& AssetDetails,TSet<FName> ClasssName);
	static TSet<FName> GetAllMaterialClassesNames();
	static TArray<UClass*> GetPreCacheClasses();
	static void DumpActiveTargetPlatforms();
	static FString GetPlatformsStr(TArray<ETargetPlatform> Platforms);
	static FPakCommandItem ParsePakResponseFileLine(const FString& Line);
	static void CopyDirectoryRecursively(const FString& SourceDirectory, const FString& DestinationDirectory);
};
