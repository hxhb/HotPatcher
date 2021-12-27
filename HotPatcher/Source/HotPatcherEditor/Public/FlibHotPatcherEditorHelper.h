// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FExternFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPatcherSpecifyAsset.h"

// RUNTIME
#include "FHotPatcherVersion.h"
#include "FChunkInfo.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "ETargetPlatform.h"
// engine header
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibHotPatcherEditorHelper.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHotPatcherEditorHelper, Log, All);

struct FExportPatchSettings;


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
class HOTPATCHEREDITOR_API UFlibHotPatcherEditorHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "HotPatch|Editor|Flib")
		static TArray<FString> GetAllCookOption();

	static void CreateSaveFileNotify(const FText& InMsg,const FString& InSavedFile,SNotificationItem::ECompletionState NotifyType = SNotificationItem::CS_Success);

	static void CheckInvalidCookFilesByAssetDependenciesInfo(const FString& InProjectAbsDir, const FString& InPlatformName, const FAssetDependenciesInfo& InAssetDependencies,TArray<FAssetDetail>& OutValidAssets,TArray<FAssetDetail>& OutInvalidAssets);

	static FChunkInfo MakeChunkFromPatchSettings(struct FExportPatchSettings const* InPatchSetting);
	static FChunkInfo MakeChunkFromPatchVerison(const FHotPatcherVersion& InPatchVersion);
	static FString GetCookAssetsSaveDir(const FString& BaseDir, const FString PacakgeName, const FString& Platform);

	static FString GetProjectCookedDir();

	static FSavePackageContext* CreateSaveContext(const ITargetPlatform* TargetPlatform,bool bUseZenLoader);
	
	//UFUNCTION(BlueprintCallable)
	static bool CookAssets(
			const TArray<FSoftObjectPath>& Assets,
			const TArray<ETargetPlatform>& Platforms,
			TFunction<void(const FString&)> PackageSavedCallback = [](const FString&){},
			TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback = [](const FString&,ETargetPlatform){},
			class TMap<ETargetPlatform,FSavePackageContext*> PlatformSavePackageContext = TMap<ETargetPlatform,FSavePackageContext*>{}
		);
	static bool CookPackages(
		const TArray<FAssetData>& AssetDatas,
		const TArray<UPackage*>& InPackage,
		const TArray<FString>& Platforms,
		TFunction<void(const FString&)> PackageSavedCallback,
		TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback,
		class TMap<ETargetPlatform,FSavePackageContext*> PlatformSavePackageContext
	);
	static bool CookPackages(
		const TArray<FAssetData>& AssetDatas,
		const TArray<UPackage*>& Packages,
		const TArray<FString>& Platforms,
		TFunction<void(const FString&)> PackageSavedCallback = [](const FString&){},
		TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback = [](const FString&,ETargetPlatform){},
		class TMap<FString,FSavePackageContext*> PlatformSavePackageContext = TMap<FString,FSavePackageContext*>{}
	);
	static bool CookPackage(
		const FAssetData& AssetData,
		UPackage* Package,
		const TArray<FString>& Platforms,
		TFunction<void(const FString&)> PackageSavedCallback = [](const FString&){},
		TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback = [](const FString&,ETargetPlatform){},
		class TMap<FString,FSavePackageContext*> PlatformSavePackageContext = TMap<FString,FSavePackageContext*>{}
	);

	static void CookChunkAssets(
		TArray<FAssetDetail> Assets,
		const TArray<ETargetPlatform>& Platforms,
		TFunction<void(const FString&,ETargetPlatform)> CookFailedCallback = [](const FString&,ETargetPlatform){},
		class TMap<ETargetPlatform,FSavePackageContext*> PlatformSavePackageContext = TMap<ETargetPlatform,FSavePackageContext*>{}
	);
	
	static ITargetPlatform* GetTargetPlatformByName(const FString& PlatformName);
	static TArray<ITargetPlatform*> GetTargetPlatformsByNames(const TArray<ETargetPlatform>& PlatformNames);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Editor|Flib")
    static FString GetUnrealPakBinary();
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Editor|Flib")
        static FString GetUECmdBinary();

	static FProcHandle DoUnrealPak(TArray<FString> UnrealPakCommandletOptions, bool block);

	static FString GetMetadataDir(const FString& ProjectDir,const FString& ProjectName,ETargetPlatform Platform);
	
	static void BackupMetadataDir(const FString& ProjectDir,const FString& ProjectName,const TArray<ETargetPlatform>& Platforms,const FString& OutDir);
	static void BackupProjectConfigDir(const FString& ProjectDir,const FString& OutDir);

	static FString ReleaseSummary(const FHotPatcherVersion& NewVersion);
	static FString PatchSummary(const FPatchVersionDiff& DiffInfo);

	static FString MakePakShortName(const FHotPatcherVersion& InCurrentVersion, const FChunkInfo& InChunkInfo, const FString& InPlatform,const FString& InRegular);

	static bool CheckSelectedAssetsCookStatus(const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg);
	static bool CheckPatchRequire(const FPatchVersionDiff& InDiff,const TArray<FString>& PlatformNames,FString& OutMsg);

	// WindowsNoEditor to Windows
	static FString Conv2IniPlatform(const FString& Platform);
	static TArray<FString> GetSupportPlatforms();

	static FString GetEncryptSettingsCommandlineOptions(const FPakEncryptSettings& EncryptSettings,const FString& PlatformName);

	static ITargetPlatform* GetPlatformByName(const FString& Name);

	// need add UNREALED_API to FAssetRegistryGenerator
	// all chunksinfo.csv / pakchunklist.txt / assetregistry.bin
	static bool GeneratorGlobalAssetRegistryData(ITargetPlatform* TargetPlatform, const TSet<FName>&, const TSet<FName>&, bool bGenerateStreamingInstallManifest = true);

	/*
	* 0x1 Add
	* 0x2 Modyfy
	*/
	static void AnalysisWidgetTree(FPatchVersionDiff& PakDiff,int32 flags = 0x1|0x2);
	
	static FPatchVersionDiff DiffPatchVersionWithPatchSetting(const struct FExportPatchSettings& PatchSetting, const FHotPatcherVersion& Base, const FHotPatcherVersion& New);

	// CurrenrVersionChunk中的过滤器会进行依赖分析，TotalChunk的不会，目的是让用户可以自己控制某个文件夹打包到哪个Pak里，而不会对该文件夹下的资源进行依赖分析
	static FChunkAssetDescribe DiffChunkWithPatchSetting(
		const struct FExportPatchSettings& PatchSetting,
		const FChunkInfo& CurrentVersionChunk,
		const FChunkInfo& TotalChunk,
		TMap<FString, FAssetDependenciesInfo>& ScanedCaches
	);
	static FChunkAssetDescribe DiffChunkByBaseVersionWithPatchSetting(
		const struct FExportPatchSettings& PatchSetting,
		const FChunkInfo& CurrentVersionChunk,
		const FChunkInfo& TotalChunk,
		const FHotPatcherVersion& BaseVersion,
		TMap<FString, FAssetDependenciesInfo>& ScanedCaches
	);
	
	static bool SerializeAssetRegistryByDetails(const FString& PlatformName,const TArray<FAssetDetail>& AssetDetails,const FString& SavePath);
	static bool SerializeAssetRegistry(const FString& PlatformName,const TArray<FString>& PackagePaths,const FString& SavePath);
	
	static FHotPatcherVersion MakeNewRelease(const FHotPatcherVersion& InBaseVersion, const FHotPatcherVersion& InCurrentVersion, FExportPatchSettings* InPatchSettings);
	static FHotPatcherVersion MakeNewReleaseByDiff(const FHotPatcherVersion& InBaseVersion, const FPatchVersionDiff& InDiff, FExportPatchSettings* InPatchSettings);

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

	static void WaitForAsyncFileWrites();

	static bool IsCanCookPackage(const FString& LongPackageName);
};
