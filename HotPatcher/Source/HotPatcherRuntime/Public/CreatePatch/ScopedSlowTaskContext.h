#pragma once
#include "HotPatcherLog.h"
// engine header
#include "CoreGlobals.h"
#include "CoreMinimal.h"
#include "Misc/ScopedSlowTask.h"
#include "ScopedSlowTaskContext.generated.h"

UCLASS()
class HOTPATCHERRUNTIME_API UScopedSlowTaskContext:public UObject
{
public:
    GENERATED_BODY()

    FORCEINLINE void init(float AmountOfWorkProgress)
    {
        if(!ProgressPtr.IsValid() && !IsRunningCommandlet())
        {
            ProgressPtr = MakeUnique<FScopedSlowTask>(AmountOfWorkProgress);
            ProgressPtr->MakeDialog();
        }
        
    }

    FORCEINLINE void EnterProgressFrame(float ExpectedWorkThisFrame, const FText& Text =FText())
    {
        if(ProgressPtr.IsValid() && !IsRunningCommandlet())
        {
            ProgressPtr->EnterProgressFrame(ExpectedWorkThisFrame,Text);
        }else
        {
            UE_LOG(LogHotPatcher,Display,TEXT("%s"),*Text.ToString());
        }
    }

    FORCEINLINE void Final()
    {
        if(ProgressPtr.IsValid() && !IsRunningCommandlet())
        {
            ProgressPtr->Destroy();
            auto Result = ProgressPtr.Release();
        }
    }

private:
    TUniquePtr<FScopedSlowTask> ProgressPtr;    
};