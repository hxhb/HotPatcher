// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FGitCommitInfo.h"
#include "FGitSourceControlRevisionData.h"

#include "NumericLimits.h"
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
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GitSourceControlEx|Flib")
		static FString GetGitBinary();

	UFUNCTION(BlueprintCallable, Category = "GitSourceControlEx|Flib")
		static bool RunGitCommandWithFiles(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);
	UFUNCTION(BlueprintCallable, Category = "GitSourceControlEx|Flib")
		static bool RunGitCommand(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "GitSourceControlEx|Flib")
		static bool DiffVersion(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, TArray<FString>& OutResault, TArray<FString>& OutErrorMessages);
	UFUNCTION(BlueprintCallable, Category = "GitSourceControlEx|Flib")
		static bool GitLog(const FString& InGitBinaey, const FString& InRepoRoot, TArray<FGitCommitInfo>& OutCommitInfo);

	UFUNCTION(BlueprintCallable, Category = "GitSourceControlEx|Flib")
		static bool GetBranchName(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutBranchName);
	UFUNCTION(BlueprintCallable, Category = "GitSourceControlEx|Flib")
		static bool GetRemoteUrl(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutRemoteUrl);


	/**
	 * Run a Git "log" command and parse it.
	 *
	 * @param	InPathToGitBinary	The path to the Git binary
	 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
	 * @param	InFile				The file to be operated on
	 * @param	bMergeConflict		In case of a merge conflict, we also need to get the tip of the "remote branch" (MERGE_HEAD) before the log of the "current branch" (HEAD)
	 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
	 * @param	OutHistory			The history of the file
	 * @param   InHistoryDepth		return history depth default is MAX_Int32
	 */
	UFUNCTION(BlueprintCallable,Category="GitSourceControlEx|Flib")
		static bool RunGetHistory(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const FString& InFile, bool bMergeConflict, TArray<FString>& OutErrorMessages, TArray<FGitSourceControlRevisionData>& OutHistory,int32 InHistoryDepth);

	UFUNCTION(BlueprintCallable, Category = "GitSourceControlEx|Flib")
		static bool GetFileLastCommit(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const FString& InFile, bool bMergeConflict, TArray<FString>& OutErrorMessages, FGitSourceControlRevisionData& OutHistory);

};
