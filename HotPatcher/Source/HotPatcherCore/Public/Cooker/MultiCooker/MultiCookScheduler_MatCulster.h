// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "FMultiCookerSettings.h"
#include "Cooker/MultiCooker/MultiCookScheduler.h"
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MultiCookScheduler_MatCulster.generated.h"

/**
 * 
 */
UCLASS(BlueprintType,Blueprintable)
class HOTPATCHERCORE_API UMultiCookScheduler_MatCulster : public UMultiCookScheduler
{
	GENERATED_BODY()
public:
	virtual TArray<FSingleCookerSettings> MultiCookScheduler_Implementation(FMultiCookerSettings& MultiCookerSettings, TArray<FAssetDetail>& AllDetails)override;
};


