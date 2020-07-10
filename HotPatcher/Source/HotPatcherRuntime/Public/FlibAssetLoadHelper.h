// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibAssetLoadHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibAssetLoadHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
		static UObject* LoadAssetByPackageName(const FString& InPackageName, FString& OutAssetType);

	UFUNCTION(BlueprintCallable)
		static FString GetObjectResource(UObject* Obj);

};
