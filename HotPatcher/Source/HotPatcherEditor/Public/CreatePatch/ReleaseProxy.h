#pragma once
#include "CreatePatch/FExportReleaseSettings.h"
#include "HotPatcherProxyBase.h"
// ENGINE HEADER
#include "CoreMinimal.h"
#include "CoreGlobals.h"
#include "ReleaseProxy.generated.h"


UCLASS()
class HOTPATCHEREDITOR_API UReleaseProxy:public UHotPatcherProxyBase
{
public:
    GENERATED_BODY()

    bool DoExport();
    FORCEINLINE bool IsRunningCommandlet()const{return ::IsRunningCommandlet();}
    FORCEINLINE virtual FExportReleaseSettings* GetSettingObject()override
    {
        return (FExportReleaseSettings*)Setting;
    }
private:
    FExportReleaseSettings* ExportReleaseSettings;
};