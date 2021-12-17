#pragma once
#include "FMultiCookerSettings.h"
#include "Cooker/HotPatcherCookerSettingBase.h"
#include "CreatePatch/HotPatcherProxyBase.h"
// engine header
#include "FPackageTracker.h"
#include "FMultiCookerSettings.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "SingleCookerProxy.generated.h"



UCLASS()
class HOTPATCHEREDITOR_API USingleCookerProxy:public UHotPatcherProxyBase
{
    GENERATED_BODY()
public:
    virtual void Init()override;
    virtual void Shutdown() override;
    virtual bool DoExport()override;
    
    virtual FSingleCookerSettings* GetSettingObject()override {return (FSingleCookerSettings*)(Setting);};
    FCookerFailedCollection& GetCookFailedAssetsCollection(){return CookFailedAssetsCollection;};
protected:
    bool HasError();
    void OnCookAssetFailed(const FString& PackagePath,ETargetPlatform Platform);
    void DoCookMission(const TArray<FAssetDetail>& Assets);
    struct FShaderCollection
    {
        FShaderCollection(USingleCookerProxy* InProxy):Proxy(InProxy)
        {
            Proxy->InitShaderLibConllections();
        }
        ~FShaderCollection()
        {
            Proxy->ShutdowShaderLibCollections();
        }
    protected:
        USingleCookerProxy* Proxy;
    };
    
    void InitShaderLibConllections();
    void ShutdowShaderLibCollections();
private:
#if WITH_PACKAGE_CONTEXT
    virtual void InitPlatformPackageContexts();
    FORCEINLINE TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> GetPlatformSavePackageContexts()const {return PlatformSavePackageContexts;}
    TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContextsRaw()const;
    bool SavePlatformBulkDataManifest(ETargetPlatform Platform);
#endif

private:
    FCookerFailedCollection CookFailedAssetsCollection;
    TSharedPtr<FPackageTracker> PackageTracker;
    TSharedPtr<FThreadWorker> WaitThreadWorker;
    TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
    TSharedPtr<struct FCookShaderCollectionProxy> PlatformCookShaderCollection;
};