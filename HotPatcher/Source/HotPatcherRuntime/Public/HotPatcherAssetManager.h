// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "HotPatcherAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UHotPatcherAssetManager : public UAssetManager
{
	GENERATED_BODY()
public:
	virtual void ScanPrimaryAssetTypesFromConfig() override;
};
