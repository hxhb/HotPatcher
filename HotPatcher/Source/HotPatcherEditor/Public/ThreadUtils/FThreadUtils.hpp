#pragma once
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
DECLARE_MULTICAST_DELEGATE(FThreadWorkerStatusDelegate);

namespace EThreadStatus
{
	enum Type
	{
		InActive,

		Busy,

		Canceling,

		Canceled,

		Completed
	};

}
class FThreadWorker : public FRunnable
{
public:
	using FCallback = TFunction<void()>;
	explicit FThreadWorker(const TCHAR *InThreadName, const FCallback& InRunFunc)
		:mThreadName(InThreadName),mRunFunc(InRunFunc),mThreadStatus(EThreadStatus::InActive)
	{}

	virtual void Execute()
	{
		if (GetThreadStatus() == EThreadStatus::InActive)
		{
			mThread = FRunnableThread::Create(this, *mThreadName);
			if (mThread)
			{
				mThreadStatus = EThreadStatus::Busy;
			}
		}
	}
	virtual void Join()
	{
		mThread->WaitForCompletion();
	}

	virtual uint32 Run()override
	{
		mRunFunc();

		return 0;
	}
	virtual void Stop()override
	{
		Cancel();
	}
	virtual void Cancel()
	{
		mThreadStatus = EThreadStatus::Canceled;
	}
	virtual void Exit()override
	{
		mThreadStatus = EThreadStatus::Completed;
	}

	virtual EThreadStatus::Type GetThreadStatus()const
	{
		return mThreadStatus;
	}
public:
	FThreadWorkerStatusDelegate CancelDelegate;
	FORCEINLINE FString GetThreadName()const {return mThreadName;}
protected:
	FString mThreadName;
	FCallback mRunFunc;
	FRunnableThread* mThread;
	volatile EThreadStatus::Type mThreadStatus;

private:
	FThreadWorker(const FThreadWorker&) = delete;
	FThreadWorker& operator=(const FThreadWorker&) = delete;

};
