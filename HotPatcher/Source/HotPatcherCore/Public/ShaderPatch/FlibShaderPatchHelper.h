// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/EngineTypes.h"
#include "CoreMinimal.h"

#include "ETargetPlatform.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibShaderPatchHelper.generated.h"



/**
 * 
 */
UCLASS()
class HOTPATCHERCORE_API UFlibShaderPatchHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable)
	static bool CreateShaderCodePatch(TArray<FString> const& OldMetaDataDirs, FString const& NewMetaDataDir, FString const& OutDir, bool bNativeFormat,bool bDeterministicShaderCodeOrder = true);

	UFUNCTION(BlueprintCallable)
	static TArray<FString> ConvDirectoryPathToStr(const TArray<FDirectoryPath>& Dirs);

	static FString ShaderExtension;
	static FString ShaderAssetInfoExtension;
	static FString StableExtension;
	
	FORCEINLINE static FString GetCodeArchiveFilename(const FString& BaseDir, const FString& LibraryName, FName Platform)
	{
		return BaseDir / FString::Printf(TEXT("ShaderArchive-%s-"), *LibraryName) + Platform.ToString() + ShaderExtension;
	}
	FORCEINLINE static FString GetShaderAssetInfoFilename(const FString& BaseDir, const FString& LibraryName, FName Platform)
	{
		return BaseDir / FString::Printf(TEXT("ShaderAssetInfo-%s-"), *LibraryName) + Platform.ToString() + ShaderAssetInfoExtension;
	}

	FORCEINLINE static FString GetStableInfoArchiveFilename(const FString& BaseDir, const FString& LibraryName, FName Platform)
	{
		return BaseDir / FString::Printf(TEXT("ShaderStableInfo-%s-"), *LibraryName) + Platform.ToString() + StableExtension;
	}
	
	static FString GetShaderStableInfoFileNameByShaderArchiveFileName(const FString& ShaderArchiveFileName);
	static FString GetShaderInfoFileNameByShaderArchiveFileName(const FString& ShaderArchiveFileName);
	
	// static void InitShaderCodeLibrary(const TArray<ETargetPlatform>& Platforms);
	// static void CleanShaderCodeLibraries(const TArray<ETargetPlatform>& Platforms);
	// void SaveGlobalShaderLibrary(const TArray<ETargetPlatform>& Platforms);
	// void SaveShaderLibrary(const ITargetPlatform* TargetPlatform, FString const& Name, const TArray<TSet<FName>>* ChunkAssignments);
};

