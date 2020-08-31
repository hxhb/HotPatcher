#pragma once
#include "ExportReleaseSettings.h"
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
    FORCEINLINE virtual UExportReleaseSettings* GetSettingObject()override
    {
        return Cast<UExportReleaseSettings>(Setting);
    }
private:
    UExportReleaseSettings* ExportReleaseSettings;
};