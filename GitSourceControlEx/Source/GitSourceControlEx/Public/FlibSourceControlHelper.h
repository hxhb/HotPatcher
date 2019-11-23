// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FGitCommitInfo.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibSourceControlHelper.generated.h"

/**
 * 
 */
UCLASS()
class GITSOURCECONTROLEX_API UFlibSourceControlHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HotPackage|Flib")
		static FString GetGitBinary();

	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool RunGitCommandWithFiles(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);
	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool RunGitCommand(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool DiffVersion(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, TArray<FString>& OutResault, TArray<FString>& OutErrorMessages);
	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool GitLog(const FString& InGitBinaey, const FString& InRepoRoot, TArray<FGitCommitInfo>& OutCommitInfo);

	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool GetBranchName(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutBranchName);
	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool GetRemoteUrl(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutRemoteUrl);
};
