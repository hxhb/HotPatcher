#pragma  once
#include "FThreadUtils.hpp"
#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformProcess.h"

class FProcWorkerThread;

DECLARE_DELEGATE_TwoParams(FOutputMsgDelegate,FProcWorkerThread*,const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FProcStatusDelegate,FProcWorkerThread*);


class FProcWorkerThread : public FThreadWorker
{
public:
	explicit FProcWorkerThread(const TCHAR *InThreadName,const FString& InProgramPath,const FString& InParams)
		: FThreadWorker(InThreadName, []() {}), mProgramPath(InProgramPath), mPragramParams(InParams)
	{}

	virtual uint32 Run()override
	{
		if (FPaths::FileExists(mProgramPath))
		{
			FPlatformProcess::CreatePipe(mReadPipe, mWritePipe);
			// std::cout << TCHAR_TO_ANSI(*mProgramPath) << " " << TCHAR_TO_ANSI(*mPragramParams) << std::endl;

			mProcessHandle = FPlatformProcess::CreateProc(*mProgramPath, *mPragramParams, false, true, true, &mProcessID, 1, NULL, mWritePipe,mReadPipe);
			if (mProcessHandle.IsValid() && FPlatformProcess::IsApplicationRunning(mProcessID))
			{
				if (ProcBeginDelegate.IsBound())
					ProcBeginDelegate.Broadcast(this);
			}

			FString Line;
			while (mProcessHandle.IsValid() && FPlatformProcess::IsApplicationRunning(mProcessID))
			{
				FString NewLine = FPlatformProcess::ReadPipe(mReadPipe);
				if (NewLine.Len() > 0)
				{
					// process the string to break it up in to lines
					Line += NewLine;
					TArray<FString> StringArray;
					int32 count = Line.ParseIntoArray(StringArray, TEXT("\n"), true);
					if (count > 1)
					{
						for (int32 Index = 0; Index < count - 1; ++Index)
						{
							StringArray[Index].TrimEndInline();
							ProcOutputMsgDelegate.ExecuteIfBound(this,StringArray[Index]);
						}
						Line = StringArray[count - 1];
						if (NewLine.EndsWith(TEXT("\n")))
						{
							Line += TEXT("\n");
						}
					}
				}
				FPlatformProcess::Sleep(0.2f);
			}
			
			bool bRunSuccessfuly = false;
			int32 ProcReturnCode;
			if (FPlatformProcess::GetProcReturnCode(mProcessHandle,&ProcReturnCode))
			{
				if (ProcReturnCode == 0)
				{
					bRunSuccessfuly = true;
				}
			}
			ProcOutputMsgDelegate.ExecuteIfBound(this,FString::Printf(TEXT("ProcWorker %s return value %d."),*mThreadName,ProcReturnCode));

			mThreadStatus = EThreadStatus::Completed;
			if (bRunSuccessfuly)
			{
				if(ProcSuccessedDelegate.IsBound())
				{
					ProcSuccessedDelegate.Broadcast(this);
				}
			}
			else
			{
				if (ProcFaildDelegate.IsBound())
				{
					ProcFaildDelegate.Broadcast(this);
				}
			}
		}
		return 0;
	}
	
	virtual void Exit()override
	{
		FThreadWorker::Exit();
	}
	
	virtual void Cancel()override
	{
		if (GetThreadStatus() != EThreadStatus::Busy)
		{
			if (CancelDelegate.IsBound())
				CancelDelegate.Broadcast();
			return;
		}
			
		mThreadStatus = EThreadStatus::Canceling;
		if (mProcessHandle.IsValid() && FPlatformProcess::IsApplicationRunning(mProcessID))
		{
			FPlatformProcess::TerminateProc(mProcessHandle, true);

			if (ProcFaildDelegate.IsBound())
			{
				ProcFaildDelegate.Broadcast(this);
			}
			mProcessHandle.Reset();
			mProcessID = 0;
		}
		mThreadStatus = EThreadStatus::Canceled;
		if (CancelDelegate.IsBound())
		{
			CancelDelegate.Broadcast();
		}
	}

	virtual uint32 GetProcesId()const { return mProcessID; }
	virtual FProcHandle GetProcessHandle()const { return mProcessHandle; }

public:
	FProcStatusDelegate ProcBeginDelegate;
	FProcStatusDelegate ProcSuccessedDelegate;
	FProcStatusDelegate ProcFaildDelegate;
	FOutputMsgDelegate ProcOutputMsgDelegate;

private:
	FRunnableThread* mThread;
	FString mProgramPath;
	FString mPragramParams;
	void* mReadPipe;
	void* mWritePipe;
	uint32 mProcessID;
	FProcHandle mProcessHandle;
};