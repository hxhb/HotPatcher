#pragma once
#include "CreatePatch/FExportReleaseSettings.h"
#include "CreatePatch/HotPatcherProxyBase.h"
// ENGINE HEADER
#include "CoreMinimal.h"
#include "CoreGlobals.h"

#include "HPLProxy.generated.h"


UCLASS()
class HOTPATCHERCORE_API UHPLProxy:public UHotPatcherProxyBase
{
public:
    GENERATED_BODY()
    virtual void Init(FPatcherEntitySettingBase* InSetting);
    virtual bool DoExport() override ;
    virtual void OnCookAndPakHPL();
    FORCEINLINE bool IsRunningCommandlet()const{return ::IsRunningCommandlet();}
};