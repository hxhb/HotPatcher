// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/PackageReload.h"
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


	UFUNCTION(BlueprintCallable, meta = (DisplayName="ReloadPackage"))
		static bool K2_ReloadPackage(const FString& InLongPackageName);

	static UPackage* ReloadPackageByName(UObject* InOuter, const FString& InLongPackageName, uint32 InReloadFlag);
};
