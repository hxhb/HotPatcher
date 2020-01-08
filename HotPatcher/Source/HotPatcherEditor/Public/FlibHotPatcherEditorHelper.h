// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FExternAssetFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPatcherSpecifyAsset.h"

// RUNTIME
#include "FHotPatcherVersion.h"

// engine header
#include "SharedPointer.h"
#include "Dom/JsonObject.h"
#include "Notifications/NotificationManager.h"
#include "Notifications/SNotificationList.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibHotPatcherEditorHelper.generated.h"

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

	static FHotPatcherVersion ExportReleaseVersionInfo(const FString& InVersionId,
		const FString& InBaseVersion,
		const FString& InDate,
		const TArray<FString>& InIncludeFilter,
		const TArray<FString>& InIgnoreFilter,
		const TArray<FPatcherSpecifyAsset>& InIncludeSpecifyAsset,
		bool InIncludeHasRefAssetsOnly = false
	);

	static void CreateSaveFileNotify(const FText& InMsg,const FString& InSavedFile);

	
	static void CheckInvalidCookFilesByAssetDependenciesInfo(const FString& InProjectAbsDir, const FString& InPlatformName, const FAssetDependenciesInfo& InAssetDependencies,TArray<FAssetDetail>& OutValidAssets,TArray<FAssetDetail>& OutInvalidAssets);
	static bool SerializeExAssetFileInfoToJsonObject(const FExternAssetFileInfo& InExFileInfo, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool SerializeExDirectoryInfoToJsonObject(const FExternDirectoryInfo& InExDirectoryInfo, TSharedPtr<FJsonObject>& OutJsonObject);
	static bool SerializeSpecifyAssetInfoToJsonObject(const FPatcherSpecifyAsset& InSpecifyAsset, TSharedPtr<FJsonObject>& OutJsonObject);

};
