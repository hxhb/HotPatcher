// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FExternAssetFileInfo.h"

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


	static void CreateSaveFileNotify(const FText& InMsg,const FString& InSavedFile);

	static bool SerializeExAssetFileInfoToJsonObject(const FExternAssetFileInfo& InExFileInfo, TSharedPtr<FJsonObject>& OutJsonObject);
};
