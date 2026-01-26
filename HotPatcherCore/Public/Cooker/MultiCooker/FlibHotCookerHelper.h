// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Resources/Version.h"
#include "CoreMinimal.h"
#include "ETargetPlatform.h"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibHotCookerHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERCORE_API UFlibHotCookerHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static FString GetCookedDir();
	static FString GetCookerBaseDir();
	// static FString GetCookerProcConfigPath(const FString& MissionName,int32 MissionID);
	static FString GetCookerProcFailedResultPath(const FString& BaseDir,const FString& MissionName, int32 MissionID);
	static FString GetProfilingCmd();
	static TSharedPtr<FCookShaderCollectionProxy> CreateCookShaderCollectionProxyByPlatform(
		const FString& ShaderLibraryName,
		TArray<ETargetPlatform> Platforms,
		bool bShareShader,
		bool bNativeShader,
		bool bMaster,
		const FString& InSavePath,
		bool bCleanSavePath = true);
	static bool IsAppleMetalPlatform(ITargetPlatform* TargetPlatform);
};
