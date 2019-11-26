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
	
	// 

	/*
	 * 返回两次提交版本的Content文件差异，返回差异文件的资源依赖
	 * @Param InGitBinaey Git的二进制文件路径
	 * @Param InRepoRoot  存储库的路径，应为项目路径
	 * @Param InBeginCommitHash Patch的基础版本的Git commit HASH值
	 * @Param InEndCommitHash Patch版本的Git commit HASH值
	 * @Param OutContentDiffInfo 两个版本之间的差异信息
	 * @Param OutDiffAssetDependencies 对所有差异的文件进行依赖分析的结果
	*/
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool DiffCommitVersionAsAssetDependencies(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, FAssetDependenciesInfo& OutContentDiffInfo, FAssetDependenciesInfo& OutDiffAssetDependencies);


	// 对版本有差异的文件做依赖分析，返回所有改动的文件的资源依赖关系,可用于与之前版本的对比，分析得到修改和新增了哪些资源
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool ParserDiffAssetDependencies(const FString& InGitBinary, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, FAssetDependenciesInfo& OutDiffDependencies);
	
	// 把非Content的文件过滤掉
	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static void FilterContentPathInArray(const TArray<FString>& InAllModuleDirList, const TArray<FString>& InAbsPathList,TArray<FString>& OutPathList);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static bool IsContentAssetPath(const TArray<FString>& InALLModuleDir, const FString& InAssetAbsPath);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static void GetAllEnabledModuleBaseDir(TArray<FString>& OutResault);

	UFUNCTION(BlueprintCallable, Category = "HotPatcher|Flib")
		static FString ConvGitTrackFilePathToAbsPath(const FString& InRepoRootDir, const FString& InTrackFile);
};
