#pragma once
// project header
#include "CreatePatch/HotPatcherSettingBase.h"
#include "HotPatcherLog.h"
#include "CreatePatch/HotPatcherContext.h"
#include "CreatePatch/TimeRecorder.h"
#include "HotPatcherDelegates.h"

// engine header
#include "CoreMinimal.h"
#include "HotPatcherProxyBase.generated.h"



UCLASS()
class HOTPATCHERCORE_API UHotPatcherProxyBase : public UObject
{
public:
    GENERATED_BODY()


    virtual void Init(FPatcherEntitySettingBase* InSetting)
    {
        SetProxySettings(InSetting);
    };
    virtual void Shutdown(){};
    FORCEINLINE virtual bool DoExport(){return false;};
    FORCEINLINE virtual FPatcherEntitySettingBase* GetSettingObject(){return Setting;};

protected:
    FORCEINLINE virtual void SetProxySettings(FPatcherEntitySettingBase* InSetting)
    {
        Setting = InSetting;
    }
public:
    FExportPakProcess OnPaking;
    FExportPakShowMsg OnShowMsg;
protected:
    FPatcherEntitySettingBase* Setting;
};

