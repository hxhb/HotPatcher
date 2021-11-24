// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Resources/Version.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/AES.h"
#include "Misc/AES.h"
#include "FlibMultiCookerHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHEREDITOR_API UFlibMultiCookerHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static FString GetMultiCookerBaseDir();
	static FString GetCookerProcConfigPath(const FString& MissionName,int32 MissionID);
	static FString GetCookerProcFailedResultPath(const FString& MissionName, int32 MissionID);
};
