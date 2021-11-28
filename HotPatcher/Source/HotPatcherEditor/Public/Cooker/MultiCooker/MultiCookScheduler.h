// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "FMultiCookerSettings.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MultiCookScheduler.generated.h"

/**
 * 
 */
UCLASS(BlueprintType,Blueprintable)
class HOTPATCHEREDITOR_API UMultiCookScheduler : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	TArray<FSingleCookerSettings> MultiCookScheduler(FMultiCookerSettings MultiCookerSettings,const TArray<FAssetDetail>& AllDetails);
	virtual TArray<FSingleCookerSettings> MultiCookScheduler_Implementation(FMultiCookerSettings MultiCookerSettings,const TArray<FAssetDetail>& AllDetails);
};


