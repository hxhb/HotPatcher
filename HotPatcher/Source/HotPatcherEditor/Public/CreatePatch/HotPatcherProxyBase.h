#pragma once
// project header
#include "CreatePatch/HotPatcherSettingBase.h"

// engine header
#include "CoreMinimal.h"
#include "HotPatcherProxyBase.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FExportPakProcess,const FString&,const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FExportPakShowMsg,const FString&);

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