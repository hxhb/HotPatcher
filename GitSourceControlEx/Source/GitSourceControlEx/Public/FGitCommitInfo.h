#pragma once

#include "CoreMinimal.h"
#include "FGitCommitInfo.generated.h"

USTRUCT(BlueprintType)
struct GITSOURCECONTROLEX_API FGitCommitInfo
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