// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGitSourceControlRevisionData.h"
/**
 * A single line of an annotated file
 */
//class FAnnotationLine
//{
//public:
//	FORCEINLINE FAnnotationLine(int32 InChangeNumber, const FString& InUserName, const FString& InLine)
//		: ChangeNumber(InChangeNumber)
//		, UserName(InUserName)
//		, Line(InLine)
//	{
//	}
//
//	int32 ChangeNumber;
//	FString UserName;
//	FString Line;
//};

/** Revision of a file, linked to a specific commit */
class GITSOURCECONTROLEX_API FGitSourceControlRevision : public TSharedFromThis<FGitSourceControlRevision, ESPMode::ThreadSafe>
{
public:
	FGitSourceControlRevision()
		: RevisionNumber(0)
	{
	}

	virtual ~FGitSourceControlRevision()
	{
	}

	/** ISourceControlRevision interface */
	virtual bool Get(const FString& InGitBinary,const FString& InRepoRootDir, FString& InOutFilename ) const;
	//virtual bool GetAnnotated( TArray<FAnnotationLine>& OutLines ) const;
	//virtual bool GetAnnotated( FString& InOutFilename ) const;
	virtual const FString& GetFilename() const;
	virtual int32 GetRevisionNumber() const;
	virtual const FString& GetRevision() const;
	virtual const FString& GetDescription() const;
	virtual const FString& GetUserName() const;
	virtual const FString& GetClientSpec() const;
	virtual const FString& GetAction() const;
	virtual TSharedPtr<class FGitSourceControlRevision, ESPMode::ThreadSafe> GetBranchSource() const;
	virtual const FDateTime& GetDate() const;
	virtual int32 GetCheckInIdentifier() const;
	virtual int32 GetFileSize() const;
	

public:

	/** The filename this revision refers to */
	FString Filename;

	/** The full hexadecimal SHA1 id of the commit this revision refers to */
	FString CommitId;

	/** The short hexadecimal SHA1 id (8 first hex char out of 40) of the commit: the string to display */
	FString ShortCommitId;

	/** The numeric value of the short SHA1 (8 first hex char out of 40) */
	int32 CommitIdNumber;

	/** The index of the revision in the history (SBlueprintRevisionMenu assumes order for the "Depot" label) */
	int32 RevisionNumber;

	/** The SHA1 identifier of the file at this revision */
	FString FileHash;

	/** The description of this revision */
	FString Description;

	/** The user that made the change */
	FString UserName;

	/** The action (add, edit, branch etc.) performed at this revision */
	FString Action;

	/** Source of move ("branch" in Perforce term) if any */
	TSharedPtr<FGitSourceControlRevision, ESPMode::ThreadSafe> BranchSource;

	/** The date this revision was made */
	FDateTime Date;

	/** The size of the file at this revision */
	int32 FileSize;
};

/** History composed of the last 100 revisions of the file */
typedef TArray< TSharedRef<FGitSourceControlRevision, ESPMode::ThreadSafe> >	TGitSourceControlHistory;
