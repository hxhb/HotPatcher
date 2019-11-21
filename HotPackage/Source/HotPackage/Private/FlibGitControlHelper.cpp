// Fill out your copyright notice in the Description page of Project Settings.


#include "FlibGitControlHelper.h"
#include "GitSourceControlUtils.h"

FString UFlibGitControlHelper::GetGitBinary()
{
	return TEXT("C:\\Program Files\\BuildPath\\Git\\bin\\git.exe");
}

bool UFlibGitControlHelper::RunGitCommandWithFiles(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages)
{
	return GitSourceControlUtils::RunCommand(InCommand, InPathToGitBinary, InRepositoryRoot, InParameters, InFiles, OutResults, OutErrorMessages);
}

bool UFlibGitControlHelper::RunGitCommand(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages)
{
	return UFlibGitControlHelper::RunGitCommandWithFiles(InCommand, InPathToGitBinary, InRepositoryRoot, InParameters, TArray<FString>{}, OutResults, OutErrorMessages);
}

bool UFlibGitControlHelper::DiffVersion(const FString& InGitBinaey, const FString& InRepoRoot, const FString& InBeginCommitHash, const FString& InEndCommitHash, TArray<FString>& OutResault, TArray<FString>& OutErrorMessages)
{
	TArray<FString> DiffParams{
		InBeginCommitHash + FString(TEXT("...")) + InEndCommitHash,
		TEXT("--name-only")
	};
	return UFlibGitControlHelper::RunGitCommand(FString(TEXT("diff")), InGitBinaey, InRepoRoot, DiffParams, OutResault, OutErrorMessages);
}

bool UFlibGitControlHelper::AllCommitInfo(const FString& InGitBinaey, const FString& InRepoRoot, TArray<FGitCommitInfo>& OutCommitInfo)
{
	TArray<FString> Params{
		TEXT("--pretty=format:\"%h;-;%s;-;%cd;-;%an\"")
	};
	TArray<FString> OutResault;
	TArray<FString> ErrorMessage;
	bool runResault = UFlibGitControlHelper::RunGitCommand(FString(TEXT("log")), InGitBinaey, InRepoRoot, Params, OutResault, ErrorMessage);
	for (auto& Item : OutResault)
	{
		TArray<FString> ItemBreaker;
		Item.ParseIntoArray(ItemBreaker, TEXT(";-;"), true);
		OutCommitInfo.Add(FGitCommitInfo{ ItemBreaker[0],ItemBreaker[1], ItemBreaker[2], ItemBreaker[3] });
	}

	return runResault;
}