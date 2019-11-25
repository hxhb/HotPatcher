// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AssetManager/FAssetDependenciesInfo.h"

#include "CoreMinimal.h"
#include "UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibPatchParserHelper.generated.h"

/**
 * 
 */
UCLASS()
class HOTPATCHERRUNTIME_API UFlibPatchParserHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static FString GetHotPatcherGitPath();

	// 返回两次提交版本的Content文件差异，文件路径为绝对路径
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool DiffCommitVersion(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, TArray<FString>& OutAbsPathResault, TArray<FString>& OutErrorMessages);
	
	// 对版本有差异的文件做依赖分析，返回所有改动的文件的资源依赖关系
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool ParserDiffAssetDependencies(const FString& InGitBinary, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, FAssetDependenciesInfo& OutDiffDependencies);
	
	// 把非Content的文件过滤掉
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static void FilterContentPathInArray(const TArray<FString>& InAllModuleDirList, const TArray<FString>& InAbsPathList,TArray<FString>& OutPathList);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool IsContentAssetPath(const TArray<FString>& InALLModuleDir, const FString& InAssetAbsPath);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static void GetAllEnabledModuleBaseDir(TArray<FString>& OutResault);


};
