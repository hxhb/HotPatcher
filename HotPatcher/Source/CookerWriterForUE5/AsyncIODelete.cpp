// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsyncIODelete.h"
#include "CoreMinimal.h"

#include "Async/Async.h"
#include "Containers/UnrealString.h"
#include "CookOnTheSide/CookOnTheFlyServer.h" // needed for DECLARE_LOG_CATEGORY_EXTERN(LogTemp,...)
#include "HAL/Event.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Math/NumericLimits.h"
#include "Misc/StringBuilder.h"
#include "Misc/Paths.h"
#include "Templates/UnrealTemplate.h"

#if WITH_ASYNCIODELETE_DEBUG
TArray<FString> FAsyncIODelete::AllTempRoots;
#endif

FAsyncIODelete::FAsyncIODelete(const FStringView& InOwnedTempRoot)
{
	SetTempRoot(InOwnedTempRoot);
}

FAsyncIODelete::~FAsyncIODelete()
{
	SetTempRoot(FStringView());
}

void FAsyncIODelete::SetTempRoot(const FStringView& InOwnedTempRoot)
{
	Teardown();

#if WITH_ASYNCIODELETE_DEBUG
	if (!TempRoot.IsEmpty())
	{
		RemoveTempRoot(*TempRoot);
	}
#endif

	TempRoot = InOwnedTempRoot;

#if WITH_ASYNCIODELETE_DEBUG
	if (!TempRoot.IsEmpty())
	{
		AddTempRoot(*TempRoot);
	}
#endif
}

void FAsyncIODelete::SetDeletesPaused(bool bInPaused)
{
	bPaused = bInPaused;
#if ASYNCIODELETE_ASYNC_ENABLED
	if (!bPaused)
	{
		IFileManager& FileManager = IFileManager::Get();
		for (const FString& DeletePath : PausedDeletes)
		{
			const bool IsDirectory = FileManager.DirectoryExists(*DeletePath);
			const bool IsFile = !IsDirectory && FileManager.FileExists(*DeletePath);
			if (!IsDirectory && !IsFile)
			{
				continue;
			}
			CreateDeleteTask(DeletePath, IsDirectory ? EPathType::Directory : EPathType::File);
		}
		PausedDeletes.Empty();
	}
#endif
}

bool FAsyncIODelete::Setup()
{
	if (bInitialized)
	{
		return true;
	}

	if (TempRoot.IsEmpty())
	{
		checkf(false, TEXT("DeleteDirectory called without having first set a TempRoot"));
		return false;
	}

#if ASYNCIODELETE_ASYNC_ENABLED
	// Delete the TempRoot directory to clear the results from any previous process using the same TempRoot that did not shut down cleanly
	uint32 ErrorCode;
	if (!DeleteTempRootDirectory(ErrorCode))
	{
		UE_LOG(LogTemp, Error, TEXT("Could not clear asyncdelete root directory '%s'.  LastError: %i."), *TempRoot, ErrorCode);
		return false;
	}

	// Create the empty directory to work in
	if (!IFileManager::Get().MakeDirectory(*TempRoot, true))
	{
		UE_LOG(LogTemp, Error, TEXT("Could not create asyncdelete root directory '%s'.  LastError: %i."), *TempRoot, FPlatformMisc::GetLastError());
		return false;
	}

	// Allocate the task event
	check(TasksComplete == nullptr);
	TasksComplete = FPlatformProcess::GetSynchEventFromPool(true /* IsManualReset */);
	check(ActiveTaskCount == 0);
	TasksComplete->Trigger(); // We have 0 tasks so the event should be in the Triggered state

	// Assert that all other teardown-transient variables were cleared by the constructor or by the previous teardown
	// TempRoot and bPaused are preserved across setup/teardown and may have any value
	check(PausedDeletes.Num() == 0);
	check(DeleteCounter == 0);
#endif

	// We are now setup and ready to create DeleteTasks
	bInitialized = true;

	return true;
}

void FAsyncIODelete::Teardown()
{
	if (!bInitialized)
	{
		return;
	}

#if ASYNCIODELETE_ASYNC_ENABLED
	// Clear task variables
	WaitForAllTasks();
	check(ActiveTaskCount == 0 && TasksComplete != nullptr && TasksComplete->Wait(0));
	FPlatformProcess::ReturnSynchEventToPool(TasksComplete);
	TasksComplete = nullptr;

	// Remove the temp directory from disk
	uint32 ErrorCode;
	if (!DeleteTempRootDirectory(ErrorCode))
	{
		// This will leave directories (and potentially files, if we were paused or if any of the asyncdeletes failed) on disk, so it is bad for users, but is not fatal for our operations.
		UE_LOG(LogTemp, Warning, TEXT("Could not delete asyncdelete root directory '%s'.  LastError: %i."), *TempRoot, ErrorCode);
	}

	// Clear delete variables; we don't need to run the tasks for the remaining pauseddeletes because synchronously deleting the temp directory above did the work they were going to do
	PausedDeletes.Empty();
	DeleteCounter = 0;
#endif

	// We are now torn down and ready for a new setup
	bInitialized = false;
}

bool FAsyncIODelete::WaitForAllTasks(float TimeLimitSeconds)
{
#if ASYNCIODELETE_ASYNC_ENABLED
	if (!bInitialized)
	{
		return true;
	}

	if (TimeLimitSeconds <= 0.f)
	{
		TasksComplete->Wait();
	}
	else
	{
		if (!TasksComplete->Wait(FTimespan::FromSeconds(TimeLimitSeconds)))
		{
			return false;
		}
	}
	check(ActiveTaskCount == 0);
#endif
	return true;
}

bool FAsyncIODelete::Delete(const FStringView& PathToDelete, EPathType ExpectedType)
{
	IFileManager& FileManager = IFileManager::Get();
	TStringBuilder<128> PathToDeleteBuffer;
	PathToDeleteBuffer << PathToDelete;
	const TCHAR* PathToDeleteSZ = PathToDeleteBuffer.ToString();

	const bool IsDirectory = FileManager.DirectoryExists(PathToDeleteSZ);
	const bool IsFile = !IsDirectory && FileManager.FileExists(PathToDeleteSZ);
	if (!IsDirectory && !IsFile)
	{
		return true;
	}
	if (ExpectedType == EPathType::Directory && !IsDirectory)
	{
		checkf(false, TEXT("DeleteDirectory called on \"%.*s\" which is not a directory."), PathToDelete.Len(), PathToDelete.GetData());
		return false;
	}
	if (ExpectedType == EPathType::File && !IsFile)
	{
		checkf(false, TEXT("DeleteFile called on \"%.*s\" which is not a file."), PathToDelete.Len(), PathToDelete.GetData());
		return false;
	}
	// Prevent the user from trying to delete our temproot or anything inside it
	FString PathToDeleteStr(PathToDelete);
	if (FPaths::IsUnderDirectory(PathToDeleteStr, TempRoot) || FPaths::IsUnderDirectory(TempRoot, PathToDeleteStr))
	{
		return false;
	}

#if ASYNCIODELETE_ASYNC_ENABLED
	if (DeleteCounter == UINT32_MAX)
	{
		Teardown();
	}
#endif
	if (!Setup())
	{
		// Setup failed; we are not able to provide asynchronous deletes; fall back to synchronous
		UE_LOG(LogTemp, Warning, TEXT("Failed to setup an async delete, falling back to synchronous delete."));
		return SynchronousDelete(PathToDeleteSZ, ExpectedType);
	}

#if ASYNCIODELETE_ASYNC_ENABLED
	const FString TempPath = FPaths::Combine(TempRoot, FString::Printf(TEXT("%u"), DeleteCounter));
	DeleteCounter++;

	const bool bReplace = true;
	const bool bEvenIfReadOnly = true;
	const bool bMoveAttributes = false;
	const bool bDoNotRetryOnError = true;
	if (!IFileManager::Get().Move(*TempPath, PathToDeleteSZ, bReplace, bEvenIfReadOnly, bMoveAttributes, bDoNotRetryOnError)) // IFileManager::Move works on either files or directories
	{
		// The move failed; try a synchronous delete as backup
		UE_LOG(LogTemp, Warning, TEXT("Failed to move path '%.*s' for async delete (LastError == %i); falling back to synchronous delete."), PathToDelete.Len(), PathToDelete.GetData(), FPlatformMisc::GetLastError());
		return SynchronousDelete(PathToDeleteSZ, ExpectedType);
	}

	if (bPaused)
	{
		PausedDeletes.Add(TempPath);
	}
	else
	{
		CreateDeleteTask(TempPath, ExpectedType);
	}
	return true;
#else
	return SynchronousDelete(PathToDeleteSZ, ExpectedType);
#endif
}

#if ASYNCIODELETE_ASYNC_ENABLED
void FAsyncIODelete::CreateDeleteTask(const FStringView& InDeletePath, EPathType PathType)
{
	{
		FScopeLock Lock(&CriticalSection);
		TasksComplete->Reset();
		ActiveTaskCount++;
	}

	AsyncThread(
		[this, DeletePath = FString(InDeletePath), PathType]() { SynchronousDelete(*DeletePath, PathType); },
		0, TPri_Normal,
		[this]() { OnTaskComplete(); });
}

void FAsyncIODelete::OnTaskComplete()
{
	FScopeLock Lock(&CriticalSection);
	check(ActiveTaskCount > 0);
	ActiveTaskCount--;
	if (ActiveTaskCount == 0)
	{
		TasksComplete->Trigger();
	}
}
#endif

bool FAsyncIODelete::SynchronousDelete(const TCHAR* InDeletePath, EPathType PathType)
{
	bool Result;
	const bool bRequireExists = false;
	if (PathType == EPathType::Directory)
	{
		const bool bTree = true;
		Result = IFileManager::Get().DeleteDirectory(InDeletePath, bRequireExists, bTree);
	}
	else
	{
		const bool bEvenIfReadOnly = true;
		Result = IFileManager::Get().Delete(InDeletePath, bRequireExists, bEvenIfReadOnly);
	}

	if (!Result)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to asyncdelete %s '%s'. LastError == %i."), PathType == EPathType::Directory ? TEXT("directory") : TEXT("file"), InDeletePath, FPlatformMisc::GetLastError());
	}
	return Result;
}

#if ASYNCIODELETE_ASYNC_ENABLED
bool FAsyncIODelete::DeleteTempRootDirectory(uint32& OutErrorCode)
{
	OutErrorCode = 0;
	IFileManager& FileManager = IFileManager::Get();
	if (!FileManager.DirectoryExists(*TempRoot))
	{
		return true;
	}

	// Since we sometimes will be creating the directory again immediately, we need to take precautions against the delayed delete of directories that
	// occurs on Windows platforms; creating a new file/directory in one that was just deleted can fail.  So we need to move-delete our TempRoot
	// in addition to move-delete our clients' directories.  Since we don't have a TempRoot to move-delete into, we create a unique sibling directory name.
	FString UniqueDirectory = FPaths::CreateTempFilename(*FPaths::GetPath(TempRoot), TEXT("DeleteTemp"), TEXT(""));

	const bool bReplace = false;
	const bool bEvenIfReadOnly = true;
	const TCHAR* DirectoryToDelete = *UniqueDirectory;
	const bool bMoveSucceeded = FileManager.Move(DirectoryToDelete, *TempRoot, bReplace, bEvenIfReadOnly);
	if (!bMoveSucceeded)
	{
		// Move failed; fallback to inplace delete
		DirectoryToDelete = *TempRoot;
	}

	const bool bRequireExists = false;
	const bool bTree = true;
	const bool bDeleteSucceeded = FileManager.DeleteDirectory(DirectoryToDelete, bRequireExists, bTree);
	if (!bDeleteSucceeded)
	{
		OutErrorCode = FPlatformMisc::GetLastError();
		if (bMoveSucceeded && !bDeleteSucceeded)
		{
			// Try to move the directory back so that we can try again to delete it next time.
			FileManager.Move(*TempRoot, DirectoryToDelete, bReplace, bEvenIfReadOnly);
		}
	}
	return bDeleteSucceeded;
}
#endif

#if WITH_ASYNCIODELETE_DEBUG
void FAsyncIODelete::AddTempRoot(const FStringView& InTempRoot)
{
	FString TempRoot(InTempRoot);
	for (FString& Existing : AllTempRoots)
	{
		checkf(!FPaths::IsUnderDirectory(Existing, TempRoot), TEXT("New FAsyncIODelete has TempRoot \"%s\" that is a subdirectory of existing TempRoot \"%s\"."), *TempRoot, *Existing);
		checkf(!FPaths::IsUnderDirectory(TempRoot, Existing), TEXT("New FAsyncIODelete has TempRoot \"%s\" that is a parent directory of existing TempRoot \"%s\"."), *TempRoot, *Existing);
	}
	AllTempRoots.Add(MoveTemp(TempRoot));
}

void FAsyncIODelete::RemoveTempRoot(const FStringView& InTempRoot)
{
	AllTempRoots.Remove(FString(InTempRoot));
}
#endif
