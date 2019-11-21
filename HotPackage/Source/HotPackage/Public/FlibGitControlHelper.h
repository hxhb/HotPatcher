// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlibGitControlHelper.generated.h"

USTRUCT(BlueprintType)
struct FGitCommitInfo
{
	GENERATED_USTRUCT_BODY()


	FORCEINLINE FGitCommitInfo(FString InHash=TEXT(""), FString InCommitContent = TEXT(""), FString InCommitTime = TEXT(""), FString InAuthor = TEXT(""))
		:mDiffHash(InHash), mCommitContent(InCommitContent),mCommitTime(InCommitTime),mAuthor(InAuthor)
	{

	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mDiffHash;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mCommitContent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mCommitTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString mAuthor;
};

UCLASS()
class HOTPACKAGE_API UFlibGitControlHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HotPackage|Flib")
		static FString GetGitBinary();

	UFUNCTION(BlueprintCallable,Category="HotPackage|Flib")
		static bool RunGitCommandWithFiles(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);
	UFUNCTION(BlueprintCallable,Category="HotPackage|Flib")
		static bool RunGitCommand(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool DiffVersion(const FString& InGitBinaey,const FString& InRepoRoot ,const FString& InBeginCommitHash, const FString& InEndCommitHash, TArray<FString>& OutResault, TArray<FString>& OutErrorMessages);
	UFUNCTION(BlueprintCallable, Category = "HotPackage|Flib")
		static bool AllCommitInfo(const FString& InGitBinaey, const FString& InRepoRoot, TArray<FGitCommitInfo>& OutCommitInfo);
};
