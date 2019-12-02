// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
};
