#pragma once
#include "FMultiCookerSettings.h"
#include "Cooker/HotPatcherCookerSettingBase.h"
#include "CreatePatch/HotPatcherProxyBase.h"
#include "HotPatcherBaseTypes.h"

// engine header
#include "BaseTypes/FPackageTracker.h"
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

USTRUCT()
struct FPackagePathSet
{
    GENERATED_BODY()

    UPROPERTY()
    TSet<FName> PackagePaths;
};

DECLARE_MULTICAST_DELEGATE(FSingleCookerEvent);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSingleCookResultEvent,const FSoftObjectPath&,ETargetPlatform);

USTRUCT()
struct FCookCluster
{
    GENERATED_BODY()
    UPROPERTY()
    TArray<FSoftObjectPath> Assets;
    UPROPERTY()
    TArray<ETargetPlatform> Platforms;
    UPROPERTY()
    bool bPreGeneratePlatformData = false;
    
    FCookResultEvent PackageSavedCallback = nullptr;
    FCookResultEvent CookFailedCallback = nullptr;
};

UCLASS()
class HOTPATCHEREDITOR_API USingleCookerProxy:public UHotPatcherProxyBase
{
    GENERATED_BODY()
public:
    virtual void Init(FPatcherEntitySettingBase* InSetting)override;
    virtual void Shutdown() override;
    virtual bool DoExport()override;
    
    virtual FSingleCookerSettings* GetSettingObject()override {return (FSingleCookerSettings*)(Setting);};
    FCookerFailedCollection& GetCookFailedAssetsCollection(){return CookFailedAssetsCollection;};
    void CookCluster(const FCookCluster& CookCluster);
    void AddCluster(const FCookCluster& CookCluster);

    TArray<FName>& GetPlatformCookAssetOrders(ETargetPlatform Platform);
public:
    FSingleCookerEvent OnCookBegin;
    FSingleCookerEvent OnCookFinished;
    FSingleCookResultEvent OnCookAssetFailed;
    FSingleCookResultEvent OnCookAssetSuccessed;
    
protected:
    bool HasError();
    void OnCookAssetFailedHandle(const FSoftObjectPath& PackagePath,ETargetPlatform Platform);
    void OnCookAssetSuccessedHandle(const FSoftObjectPath& PackagePath,ETargetPlatform Platform);
    void DoCookMission(const TArray<FAssetDetail>& Assets);
    void BulkDataManifest();
    void IoStoreManifest();
    
    void InitShaderLibConllections();
    void ShutdowShaderLibCollections();
private:
#if WITH_PACKAGE_CONTEXT
    // virtual void InitPlatformPackageContexts();
    FORCEINLINE TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>>& GetPlatformSavePackageContexts() {return PlatformSavePackageContexts;}
    TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContextsRaw();
    // bool SavePlatformBulkDataManifest(ETargetPlatform Platform);
#endif

    
    void MarkAssetCooked(const FSoftObjectPath& PackagePath,ETargetPlatform Platform);
    
private:
    TArray<FCookCluster> CookClusters;
    FCookerFailedCollection CookFailedAssetsCollection;
    TSharedPtr<FPackageTracker> PackageTracker;
    TSharedPtr<FThreadWorker> WaitThreadWorker;
    TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
    TSharedPtr<struct FCookShaderCollectionProxy> PlatformCookShaderCollection;
    FPackagePathSet PackagePathSet;
    TMap<ETargetPlatform,TArray<FName>> CookAssetOrders;
};