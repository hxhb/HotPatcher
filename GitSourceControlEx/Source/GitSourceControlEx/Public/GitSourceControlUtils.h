// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GitSourceControlRevision.h"
#include "FGitVersionEx.h"


namespace GitSourceControlUtils
{

/**
 * Find the path to the Git binary, looking into a few places (standalone Git install, and other common tools embedding Git)
 * @returns the path to the Git binary if found, or an empty string.
 */
FString GITSOURCECONTROLEX_API FindGitBinaryPath();

/**
 * Run a Git "version" command to check the availability of the binary.
 * @param InPathToGitBinary		The path to the Git binary
 * @param OutGitVersion         If provided, populate with the git version parsed from "version" command
 * @returns true if the command succeeded and returned no errors
 */
bool GITSOURCECONTROLEX_API CheckGitAvailability(const FString& InPathToGitBinary, FGitVersionEx* OutVersion = nullptr);

/**
 * Parse the output from the "version" command into GitMajorVersion and GitMinorVersion.
 * @param InVersionString       The version string returned by `git --version`
 * @param OutVersion            The FGitVersionEx to populate
 */
 void GITSOURCECONTROLEX_API ParseGitVersion(const FString& InVersionString, FGitVersionEx* OutVersion);

 FGitVersionEx GITSOURCECONTROLEX_API GetGitVersion(const FString& InGitBinary);

/** 
 * Check git for various optional capabilities by various means.
 * @param InPathToGitBinary		The path to the Git binary
 * @param OutGitVersion			If provided, populate with the git version parsed from "version" command
 */
void GITSOURCECONTROLEX_API FindGitCapabilities(const FString& InPathToGitBinary, FGitVersionEx *OutVersion);

/**
 * Run a Git "lfs" command to check the availability of the "Large File System" extension.
 * @param InPathToGitBinary		The path to the Git binary
 * @param OutGitVersion			If provided, populate with the git version parsed from "version" command
 */
 void GITSOURCECONTROLEX_API FindGitLfsCapabilities(const FString& InPathToGitBinary, FGitVersionEx *OutVersion);

/**
 * Find the root of the Git repository, looking from the provided path and upward in its parent directories
 * @param InPath				The path to the Game Directory (or any path or file in any git repository)
 * @param OutRepositoryRoot		The path to the root directory of the Git repository if found, else the path to the ProjectDir
 * @returns true if the command succeeded and returned no errors
 */
bool GITSOURCECONTROLEX_API FindRootDirectory(const FString& InPath, FString& OutRepositoryRoot);

/**
 * Get Git config user.name & user.email
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory (can be empty)
 * @param	OutUserName			Name of the Git user configured for this repository (or globaly)
 * @param	OutEmailName		E-mail of the Git user configured for this repository (or globaly)
 */
void GITSOURCECONTROLEX_API GetUserConfig(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutUserName, FString& OutUserEmail);

/**
 * Get Git current checked-out branch
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	OutBranchName		Name of the current checked-out branch (if any, ie. not in detached HEAD)
 * @returns true if the command succeeded and returned no errors
 */
bool GITSOURCECONTROLEX_API GetBranchName(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutBranchName);

/**
 * Get the URL of the "origin" defaut remote server
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	OutRemoteUrl		URL of "origin" defaut remote server
 * @returns true if the command succeeded and returned no errors
 */
bool GITSOURCECONTROLEX_API GetRemoteUrl(const FString& InPathToGitBinary, const FString& InRepositoryRoot, FString& OutRemoteUrl);

/**
 * Run a Git command - output is a string TArray.
 *
 * @param	InCommand			The Git command - e.g. commit
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory (can be empty)
 * @param	InParameters		The parameters to the Git command
 * @param	InFiles				The files to be operated on
 * @param	OutResults			The results (from StdOut) as an array per-line
 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
 * @returns true if the command succeeded and returned no errors
 */
bool GITSOURCECONTROLEX_API RunCommand(const FString& InCommand, const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);

/**
 * Run a Git "commit" command by batches.
 *
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	InParameter			The parameters to the Git commit command
 * @param	InFiles				The files to be operated on
 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
 * @returns true if the command succeeded and returned no errors
 */
bool GITSOURCECONTROLEX_API RunCommit(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const TArray<FString>& InParameters, const TArray<FString>& InFiles, TArray<FString>& OutResults, TArray<FString>& OutErrorMessages);


/**
 * Run a Git "log" command and parse it.
 *
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	InFile				The file to be operated on
 * @param	bMergeConflict		In case of a merge conflict, we also need to get the tip of the "remote branch" (MERGE_HEAD) before the log of the "current branch" (HEAD)
 * @param	OutErrorMessages	Any errors (from StdErr) as an array per-line
 * @param	OutHistory			The history of the file
 */
bool GITSOURCECONTROLEX_API RunGetHistory(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const FString& InFile, bool bMergeConflict, TArray<FString>& OutErrorMessages, TGitSourceControlHistory& OutHistory, int32 InHistoryDepth = MAX_int32);

/**
 * Run a Git "cat-file" command to dump the binary content of a revision into a file.
 *
 * @param	InPathToGitBinary	The path to the Git binary
 * @param	InRepositoryRoot	The Git repository from where to run the command - usually the Game directory
 * @param	InParameter			The parameters to the Git show command (rev:path)
 * @param	InDumpFileName		The temporary file to dump the revision
 * @returns true if the command succeeded and returned no errors
*/
bool RunDumpToFile(const FString& InPathToGitBinary, const FString& InRepositoryRoot, const FString& InParameter, const FString& InDumpFileName);


}
