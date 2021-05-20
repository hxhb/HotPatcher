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

	UFUNCTION(BlueprintCallable)
		static bool CookAssets(const TArray<FSoftObjectPath>& Assets, const TArray<ETargetPlatform>& Platforms, const FString& SavePath = TEXT(""));
	static bool CookPackages(const TArray<FAssetData>& AssetDatas,TArray<UPackage*>& Packages, const TArray<FString>& Platforms, const FString& SavePath);
	static bool CookPackage(const FAssetData& AssetData,UPackage* Package, const TArray<FString>& Platforms, const FString& SavePath);

	static void CookChunkAssets(
		const FPatchVersionDiff& DiffInfo,
		const FChunkInfo& Chunk,
		const TArray<ETargetPlatform>& Platforms,
		TMap<FString,FAssetDependenciesInfo>& ScanedCaches
	);
	
	static ITargetPlatform* GetTargetPlatformByName(const FString& PlatformName);
	static TArray<ITargetPlatform*> GetTargetPlatformsByNames(const TArray<ETargetPlatform>& PlatformNames);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Editor|Flib")
    static FString GetUnrealPakBinary();
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Editor|Flib")
        static FString GetUE4CmdBinary();

	static FProcHandle DoUnrealPak(TArray<FString> UnrealPakOptions, bool block);

	static FString GetMetadataDir(const FString& ProjectDir,const FString& ProjectName,ETargetPlatform Platform);
	
	static void BackupMetadataDir(const FString& ProjectDir,const FString& ProjectName,const TArray<ETargetPlatform>& Platforms,const FString& OutDir);

	static FString ReleaseSummary(const FHotPatcherVersion& NewVersion);
	static FString PatchSummary(const FPatchVersionDiff& DiffInfo);

	static FString MakePakShortName(const FHotPatcherVersion& InCurrentVersion, const FChunkInfo& InChunkInfo, const FString& InPlatform,const FString& InRegular);

	static bool CheckSelectedAssetsCookStatus(const TArray<FString>& PlatformNames, const FAssetDependenciesInfo& SelectedAssets, FString& OutMsg);
	static bool CheckPatchRequire(const FPatchVersionDiff& InDiff,const TArray<FString>& PlatformNames,FString& OutMsg);
};
