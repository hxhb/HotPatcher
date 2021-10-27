// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "ShaderCodeLibrary.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibShaderCodeLibraryHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHEREDITOR_API UFlibShaderCodeLibraryHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static TArray<FShaderCodeLibrary::FShaderFormatDescriptor> GetShaderFormatsWithStableKeys(const TArray<FName>& ShaderFormats,bool bNeedShaderStableKeys = true,bool bNeedsDeterministicOrder = true);
	static TArray<FName> GetShaderFormatsByTargetPlatform(ITargetPlatform* TargetPlatform);
	static FString GenerateShaderCodeLibraryName(FString const& Name, bool bIsIterateSharedBuild);
	static void SaveShaderLibrary(const ITargetPlatform* TargetPlatform, FString const& Name, const TArray<TSet<FName>>* ChunkAssignments);

	static FString ConvertToFullSandboxPath( const FString &FileName, bool bForWrite = false );
	static FString ConvertToFullSandboxPath( const FString &FileName, bool bForWrite, const FString& PlatformName );
};
