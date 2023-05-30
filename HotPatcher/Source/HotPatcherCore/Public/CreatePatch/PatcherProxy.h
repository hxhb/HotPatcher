#pragma once

#include "CreatePatch/FExportPatchSettings.h"
#include "FPatchVersionDiff.h"
#include "HotPatcherProxyBase.h"
#include "ThreadUtils/FThreadUtils.hpp"

// engine header
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Cooker/MultiCooker/FCookShaderCollectionProxy.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "PatcherProxy.generated.h"

using FPatchWorkerType = TFunction<bool(FHotPatcherPatchContext&)>;
using FPatchWorkers = TMap<FString,FPatchWorkerType>;

DECLARE_MULTICAST_DELEGATE_FourParams(FOnPakListGenerated,FHotPatcherPatchContext&,FChunkInfo&,ETargetPlatform,TArray<FPakCommand>&);
DECLARE_MULTICAST_DELEGATE_FourParams(FAddPatchWorkerEvent,FHotPatcherPatchContext&,FChunkInfo&,ETargetPlatform,TArray<FPakCommand>&);


UCLASS()
class HOTPATCHERCORE_API UPatcherProxy:public UHotPatcherProxyBase
{
    GENERATED_UCLASS_BODY()
public:
    virtual void Init(FPatcherEntitySettingBase* InSetting)override;
    virtual void Shutdown() override;
    
    virtual bool DoExport() override;
    virtual FExportPatchSettings* GetSettingObject()override{ return (FExportPatchSettings*)Setting; }
    bool CanExportPatch() const;

    FORCEINLINE_DEBUGGABLE void AddPatchWorker(const FString& WorkerName,const FPatchWorkerType& Worker)
    {
        PatchWorkers.Add(WorkerName,Worker);
    }
    FORCEINLINE const FPatchWorkers& GetPatchWorkers()const{ return PatchWorkers; }
    FORCEINLINE FPatherResult& GetPatcherResult(){ return PatcherResult; }

public:
    FOnPakListGenerated OnPakListGenerated;
    
protected:
    FPatchWorkers PatchWorkers;
private:
    TSharedPtr<FHotPatcherPatchContext> PatchContext;
    FPatherResult PatcherResult;
};
