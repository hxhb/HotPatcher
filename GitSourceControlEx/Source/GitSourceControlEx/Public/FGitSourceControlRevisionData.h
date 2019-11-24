#pragma once

#include "CoreMinimal.h"
#include "GitSourceControlRevision.h"
#include "FGitSourceControlRevisionData.generated.h"

USTRUCT(BlueprintType)
struct GITSOURCECONTROLEX_API FGitSourceControlRevisionData
{
	GENERATED_USTRUCT_BODY()

	/** The filename this revision refers to */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString Filename;

	/** The full hexadecimal SHA1 id of the commit this revision refers to */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString CommitId;

	/** The short hexadecimal SHA1 id (8 first hex char out of 40) of the commit: the string to display */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString ShortCommitId;

	/** The numeric value of the short SHA1 (8 first hex char out of 40) */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 CommitIdNumber;

	/** The index of the revision in the history (SBlueprintRevisionMenu assumes order for the "Depot" label) */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 RevisionNumber;

	/** The SHA1 identifier of the file at this revision */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString FileHash;

	/** The description of this revision */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString Description;

	/** The user that made the change */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString UserName;

	/** The action (add, edit, branch etc.) performed at this revision */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString Action;

	///** Source of move ("branch" in Perforce term) if any */
	//UPROPERTY(EditAnywhere,BlueprintReadWrite)
	//TSharedPtr<FGitSourceControlRevision, ESPMode::ThreadSafe> BranchSource;

	/** The date this revision was made */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString Date;

	/** The size of the file at this revision */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 FileSize;
};