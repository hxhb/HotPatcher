// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibReflectionHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibReflectionHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	static FProperty* GetPropertyByName(UClass* Class, FName PropertyName);
	
	UFUNCTION(BlueprintCallable)
	static FString ExportPropertyToText(UObject* Object,FName PropertyName);
	UFUNCTION(BlueprintCallable)
	static bool ImportPropertyValueFromText(UObject* Object,FName PropertyName,const FString& Text);
};
