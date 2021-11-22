#pragma once


#include "Model/FHotPatcherCreatePatchModel.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "SHotPatcherInformations.h"
#include "SHotPatcherPatchableBase.h"
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
#include "Widgets/Text/SMultiLineEditableText.h"

#include "PatcherProxy.generated.h"

DECLARE_MULTICAST_DELEGATE_FourParams(FOnPakListGenerated,FHotPatcherPatchContext&,FChunkInfo&,ETargetPlatform,TArray<FPakCommand>&);

UCLASS()
class HOTPATCHEREDITOR_API UPatcherProxy:public UHotPatcherProxyBase
{
    GENERATED_UCLASS_BODY()
public:
    bool CanExportPatch() const;
    virtual bool DoExport() override;
    virtual FExportPatchSettings* GetSettingObject()override{ return (FExportPatchSettings*)Setting; }
    FOnPakListGenerated OnPakListGenerated;

#if WITH_PACKAGE_CONTEXT
    virtual void InitPlatformPackageContexts();
    FORCEINLINE TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> GetPlatformSavePackageContexts()const {return PlatformSavePackageContexts;}
    FORCEINLINE TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContextsRaw()const
    {
        TMap<ETargetPlatform,FSavePackageContext*> result;
        TArray<ETargetPlatform> Keys;
        GetPlatformSavePackageContexts().GetKeys(Keys);
        for(const auto& Key:Keys)
        {
            result.Add(Key,GetPlatformSavePackageContexts().Find(Key)->Get());
        }
        return result;
    }
    bool SavePlatformBulkDataManifest(ETargetPlatform Platform);
    FSavePackageContext* CreateSaveContext(const ITargetPlatform* TargetPlatform,bool bUseZenLoader);
#endif
    
private:
    TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
    
private:
    TSharedPtr<FHotPatcherPatchContext> PatchContext;

    TSharedPtr<FThreadWorker> ThreadWorker;
};
