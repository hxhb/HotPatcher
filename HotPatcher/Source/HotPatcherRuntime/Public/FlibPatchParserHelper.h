// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AssetManager/FAssetDependenciesInfo.h"

#include "CoreMinimal.h"
#include "UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibPatchParserHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibPatchParserHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static TArray<FString> GetAvailableMaps(FString GameName, bool IncludeEngineMaps, bool Sorted);

	
};
