// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FExternAssetFileInfo.h"
#include "FExternDirectoryInfo.h"
#include "FPatcherSpecifyAsset.h"

// RUNTIME
#include "FHotPatcherVersion.h"

// engine header
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
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

	static void CreateSaveFileNotify(const FText& InMsg,const FString& InSavedFile);

	static void CheckInvalidCookFilesByAssetDependenciesInfo(const FString& InProjectAbsDir, const FString& InPlatformName, const FAssetDependenciesInfo& InAssetDependencies,TArray<FAssetDetail>& OutValidAssets,TArray<FAssetDetail>& OutInvalidAssets);

	static bool SerializePatchConfigToJsonObject(const class UExportPatchSettings*const InPatchSetting, TSharedPtr<FJsonObject>& OutJsonObject);
	static class UExportPatchSettings* DeserializePatchConfig(class UExportPatchSettings* InNewSetting,const FString& InContent);
	static bool SerializeReleaseConfigToJsonObject(const class UExportReleaseSettings*const InReleaseSetting, TSharedPtr<FJsonObject>& OutJsonObject);
	static class UExportReleaseSettings* DeserializeReleaseConfig(class UExportReleaseSettings* InNewSetting, const FString& InContent);


	static FChunkInfo MakeChunkFromPatchSettings(const UExportPatchSettings* InPatchSetting);
	static FChunkInfo MakeChunkFromPatchVerison(const FHotPatcherVersion& InPatchVersion);
};
