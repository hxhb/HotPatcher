// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Resources/Version.h"
#include "ShaderCodeLibrary.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibShaderCodeLibraryHelper.generated.h"

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 26
	#define SHADER_COOKER_CLASS FShaderLibraryCooker
#else
	#define SHADER_COOKER_CLASS FShaderCodeLibrary
#endif

struct FShaderCodeFormatMap
{
	ITargetPlatform* Platform;
	bool bIsNative;
	FString SaveBaseDir;
	struct FShaderFormatNameFiles
	{
		FString ShaderName;
		TArray<FString> Files;
	};
	// etc GLSL_ES3_1_ANDROID something files
	// SF_METAL something files
	TMap<FString,FShaderFormatNameFiles> ShaderCodeTypeFilesMap;
};
//
// struct FMergeShaderCollectionProxy
// {
// 	FMergeShaderCollectionProxy(const TArray<FShaderCodeFormatMap>& InShaderCodeFiles);
// 	virtual ~FMergeShaderCollectionProxy();
// 	void Init();
// 	void Shutdown();
// private:
// 	TArray<FShaderCodeFormatMap> ShaderCodeFiles;
// };

/**
 * 
 */
UCLASS()
class HOTPATCHERCORE_API UFlibShaderCodeLibraryHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
	static TArray<SHADER_COOKER_CLASS::FShaderFormatDescriptor> GetShaderFormatsWithStableKeys(const TArray<FName>& ShaderFormats,bool bNeedShaderStableKeys = true,bool bNeedsDeterministicOrder = false);
#endif
	static TArray<FName> GetShaderFormatsByTargetPlatform(ITargetPlatform* TargetPlatform);
	static FString GenerateShaderCodeLibraryName(FString const& Name, bool bIsIterateSharedBuild);
	static bool SaveShaderLibrary(const ITargetPlatform* TargetPlatform,TArray<FName> ShaderFormats, FString const& Name, const FString& ShaderCodeDir,const FString& RootMetaDataPath, bool bMaster);
	static bool SaveShaderLibrary(const ITargetPlatform* TargetPlatform, const TArray<TSet<FName>>* ChunkAssignments, FString const& Name, const FString&
	                              SaveBaseDir, bool bMaster);
	static TArray<FString> FindCookedShaderLibByPlatform(const FString& PlatfomName,const FString& Directory,bool bRecursive = false);
	static TArray<FString> FindCookedShaderLibByShaderFrmat(const FString& ShaderFormatName,const FString& Directory);

	static void WaitShaderCompilingComplete();
	static void CancelMaterialShaderCompile(UMaterialInterface* MaterialInterface);
	
	static void CleanShaderWorkerDir();

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
	static bool RHISupportsNativeShaderLibraries(const FStaticShaderPlatform Platform);
};
