// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AssetManager/FAssetDependenciesInfo.h"
#include "AssetManager/FAssetDependenciesDetail.h"

#include "AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibAssetManageHelper.generated.h"





UCLASS()
class HOTPACKAGE_API UFlibAssetManageHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependencies(const FString& InAsset, FAssetDependenciesInfo& OutDependices);

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static void GetAssetDependenciesBySoftRef(TSoftObjectPtr<UObject> InAssetRef, FAssetDependenciesInfo& OutDependicesInfo);
	
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static TArray<FString> GetAssetAllDependencies(const FString& InAsset);
	static void GatherAssetDependicesInfoRecursively(
		FAssetRegistryModule& InAssetRegistryModule,
		const FString& InTargetLongPackageName,
		FAssetDependenciesInfo& OutDependencies
	);


	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static FString ConvGameAssetRelativePathToAbs(const FString& InProjectDir, const FString& InRelativePath);
	UFUNCTION(BlueprintCallable, Category = "GWorld|Flib|AssetManager")
		static FString ConvGameAssetAbsPathToRelative(const FString& InProjectDir, const FString& InAbsPath);


	static FString ConvPath_Slash2BackSlash(const FString& InPath);
};
