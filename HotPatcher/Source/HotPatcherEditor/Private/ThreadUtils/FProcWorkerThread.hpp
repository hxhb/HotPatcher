#pragma  once
#include "FThreadUtils.hpp"
#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformProcess.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOutputMsgDelegate, const FString&);
DECLARE_MULTICAST_DELEGATE(FProcStatusDelegate);


class FProcWorkerThread : public FThread
{
public:
	explicit FProcWorkerThread(const TCHAR *InThreadName,const FString& InProgramPath,const FString& InParams)
		: FThread(InThreadName, []() {}), mProgramPath(InProgramPath), mPragramParams(InParams)
	{}

	virtual uint32 Run()override
	{
		if (FPaths::FileExists(mProgramPath))
		{
			FPlatformProcess::CreatePipe(mReadPipe, mWritePipe);
			// std::cout << TCHAR_TO_ANSI(*mProgramPath) << " " << TCHAR_TO_ANSI(*mPragramParams) << std::endl;

			mProcessHandle = FPlatformProcess::CreateProc(*mProgramPath, *mPragramParams, false, true, true, &mProcessID, 0, NULL, mWritePipe,mReadPipe);
			if (mProcessHandle.IsValid() && FPlatformProcess::IsApplicationRunning(mProcessID))
			{
				ProcBeginDelegate.Broadcast();
			}

			while (mProcessHandle.IsValid() && FPlatformProcess::IsApplicationRunning(mProcessID))
			{
				FPlatformProcess::Sleep(0.0f);

				FString NewLine = FPlatformProcess::ReadPipe(mReadPipe);
				if (NewLine.Len() > 0)
				{
					ProcOutputMsgDelegate.Broadcast(NewLine);
				}
			}

			int32 ProcReturnCode;
			if (FPlatformProcess::GetProcReturnCode(mProcessHandle,&ProcReturnCode))
			{
				if (ProcReturnCode == 0)
				{
					ProcSuccessedDelegate.Broadcast();
				}
				else
				{
					ProcFaildDelegate.Broadcast();
				}
			}
			
		}
		mThreadStatus = EThreadStatus::Completed;
		return 0;
	}
	virtual void Exit()override
	{
		if (mProcessHandle.IsValid())
		{
			
		}
	}
	virtual void Cancel()override
	{
		if (GetThreadStatus() != EThreadStatus::Busy)
			return;
		mThreadStatus = EThreadStatus::Canceling;
		if (mProcessHandle.IsValid() && FPlatformProcess::IsApplicationRunning(mProcessID))
		{
			FPlatformProcess::TerminateProc(mProcessHandle, true);
			mProcessHandle.Reset();
			mProcessID = 0;
		}
		mThreadStatus = EThreadStatus::Canceled;
		CancelDelegate.Broadcast();
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