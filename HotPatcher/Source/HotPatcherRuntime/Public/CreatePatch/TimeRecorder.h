#pragma once

#include "CoreMinimal.h"
#include "HotPatcherLog.h"

struct HOTPATCHERRUNTIME_API TimeRecorder
{
    TimeRecorder(const FString& InDisplay=TEXT(""),bool InAuto = true):Display(InDisplay),bAuto(InAuto)
    {
        if(bAuto)
        {
            Begin(InDisplay);
        }
        
    }
    ~TimeRecorder()
    {
        if(bAuto)
        {
            End();
        }
    }
    void Begin(const FString& InDisplay)
    {
        Display = InDisplay;
        BeginTime = FDateTime::Now();
    }
    void End()
    {
        CurrentTime = FDateTime::Now();
        UsedTime = CurrentTime-BeginTime;
        UE_LOG(LogHotPatcher,Display,TEXT("----Time Recorder----: %s %s"),*Display,*UsedTime.ToString());
    }
public:
    FDateTime BeginTime;
    FDateTime CurrentTime;
    FTimespan UsedTime;
    FString Display;
    bool bAuto = false;
};
