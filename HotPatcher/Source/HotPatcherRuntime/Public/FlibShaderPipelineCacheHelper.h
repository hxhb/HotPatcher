// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibShaderPipelineCacheHelper.generated.h"


UENUM(BlueprintType)
enum class EPSOSaveMode : uint8
{
	Incremental = 0, // Fast(er) approach which saves new entries incrementally at the end of the file, replacing the table-of-contents, but leaves everything else alone.
	BoundPSOsOnly = 1, // Slower approach which consolidates and saves all PSOs used in this run of the program, removing any entry that wasn't seen, and sorted by the desired sort-mode.
	SortedBoundPSOs = 2 // Slow save consolidates all PSOs used on this device that were never part of a cache file delivered in game-content, sorts entries into the desired order and will thus read-back from disk.
};

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibShaderPipelineCacheHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="")
	static bool LoadShaderPipelineCache(const FString& Name);

	UFUNCTION(BlueprintCallable, Category="")
	static bool EnableShaderPipelineCache(bool bEnable);
	
	UFUNCTION(BlueprintCallable, Category="")
	static bool SavePipelineFileCache(EPSOSaveMode Mode);

	// r.ShaderPipelineCache.LogPSO
	// 1 Logs new PSO entries into the file cache and allows saving.
	UFUNCTION(BlueprintCallable, Category="")
	static bool EnableLogPSO(bool bEnable);
	
	UFUNCTION(BlueprintCallable, Category="")
	static bool EnableSaveBoundPSOLog(bool bEnable);

	UFUNCTION(BlueprintCallable, Category="")
	static bool IsEnabledUsePSO();
	UFUNCTION(BlueprintCallable, Category="")
	static bool IsEnabledLogPSO();
	UFUNCTION(BlueprintCallable, Category="")
	static bool IsEnabledSaveBoundPSOLog();
	
};
