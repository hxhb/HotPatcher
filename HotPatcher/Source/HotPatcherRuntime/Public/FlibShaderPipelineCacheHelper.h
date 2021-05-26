// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibShaderPipelineCacheHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibShaderPipelineCacheHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static bool LoadShaderPipelineCache(const FString& Name);

	UFUNCTION(BlueprintCallable)
	static bool EnableShaderPipelineCache(bool bEnable);
};
