// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibSourceControlHelper.h"
#include "GitSourceControlUtils.h"

FString UFlibSourceControlHelper::GetGitBinary()
{
	return GitSourceControlUtils::FindGitBinaryPath();
}

bool UFlibSourceControlHelper::RunGitCommandWithFiles(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages)
{
	return GitSourceControlUtils::RunCommand(InCommand, InPathToGitBinary, InRepositoryRoot, InParameters, InFiles, OutResults, OutErrorMessages);
}

bool UFlibSourceControlHelper::RunGitCommand(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages)
{
	return UFlibSourceControlHelper::RunGitCommandWithFiles(InCommand, InPathToGitBinary, InRepositoryRoot, InParameters, TArray<FString>{}, OutResults, OutErrorMessages);
}

bool UFlibSourceControlHelper::DiffVersion(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, TArray<FString>& OutResault, TArray<FString>& OutErrorMessages)
{
	TArray<FString> DiffParams{
		InBeginCommitHash + FString(TEXT("...")) + InEndCommitHash,
		TEXT("--name-only"),
		TEXT("-r")
	};
	return UFlibSourceControlHelper::RunGitCommand(FString(TEXT("diff")), InGitBinaey, InRepoRoot, DiffParams, OutResault, OutErrorMessages);
}

bool UFlibSourceControlHelper::GitLog(const FString& InGitBinaey, const FString& InRepoRoot, TArray<FGitCommitInfo>& OutCommitInfo)
{
	TArray<FString> Params{
		TEXT("--pretty=format:\"%h;-;%s;-;%cd;-;%an\"")
	};
	TArray<FString> OutResault;
	TArray<FString> ErrorMessage;
	bool runResault = UFlibSourceControlHelper::RunGitCommand(FString(TEXT("log")), InGitBinaey, InRepoRoot, Params, OutResault, ErrorMessage);
	for (auto& Item : OutResault)
	{
		TArray<FString> ItemBreaker;
		Item.ParseIntoArray(ItemBreaker, TEXT(";-;"), true);
		OutCommitInfo.Add(FGitCommitInfo{ ItemBreaker[0],ItemBreaker[1], ItemBreaker[2], ItemBreaker[3] });
	}

	return runResault;
}

bool UFlibSourceControlHelper::GetBranchName(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutBranchName)
{
	return GitSourceControlUtils::GetBranchName(InPathToGitBinary, InRepositoryRoot, OutBranchName);
}

bool UFlibSourceControlHelper::GetRemoteUrl(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutRemoteUrl)
{
	return GitSourceControlUtils::GetRemoteUrl(InPathToGitBinary, InRepositoryRoot, OutRemoteUrl);
}