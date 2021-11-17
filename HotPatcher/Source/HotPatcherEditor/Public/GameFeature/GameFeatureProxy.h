#pragma once
#include "ThreadUtils/FThreadUtils.hpp"
#include "FGameFeaturePackagerSettings.h"
// engine header
#include "Templates/SharedPointer.h"
#include "CreatePatch/HotPatcherProxyBase.h"
#include "GameFeatureProxy.generated.h"


UCLASS()
class HOTPATCHEREDITOR_API UGameFeatureProxy:public UHotPatcherProxyBase
{
    GENERATED_BODY()
public:
    virtual bool DoExport() override;
    virtual FGameFeaturePackagerSettings* GetSettingObject()override{ return (FGameFeaturePackagerSettings*)Setting; }

private:
    TSharedPtr<FExportPatchSettings> PatchSettings;
    TSharedPtr<FThreadWorker> ThreadWorker;
};
