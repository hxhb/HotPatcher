#pragma once
// project header
#include "CreatePatch/HotPatcherSettingBase.h"
#include "HotPatcherLog.h"
#include "CreatePatch/HotPatcherContext.h"
#include "CreatePatch/TimeRecorder.h"

// engine header
#include "CoreMinimal.h"
#include "HotPatcherProxyBase.generated.h"

UCLASS()
class HOTPATCHEREDITOR_API UHotPatcherProxyBase : public UObject
{
public:
    GENERATED_BODY()

    FORCEINLINE virtual void SetProxySettings(FHotPatcherSettingBase* InSetting)
    {
        Setting = InSetting;
    }
    FORCEINLINE virtual bool DoExport(){return false;};
    FORCEINLINE virtual FHotPatcherSettingBase* GetSettingObject(){return Setting;};
    
public:
    FExportPakProcess OnPaking;
    FExportPakShowMsg OnShowMsg;
protected:
    FHotPatcherSettingBase* Setting;
};

