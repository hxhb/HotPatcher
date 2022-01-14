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

    FCookActionCallback CookActionCallback;
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
    void CookClusterAsync(const FCookCluster& CookCluster);
    void CookClusterSync(const FCookCluster& CookCluster);
    void CookCluster(const FCookCluster& CookCluster, bool bAsync);
    void AddCluster(const FCookCluster& CookCluster);

    TArray<FName>& GetPlatformCookAssetOrders(ETargetPlatform Platform);
    TSet<FName> GetAdditionalAssets();
    
public:
    FSingleCookerEvent OnCookBegin;
    FSingleCookerEvent OnCookFinished;
    FSingleCookResultEvent OnCookAssetBegin;
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

    void MarkAssetCooked(const FSoftObjectPath& PackagePath,ETargetPlatform Platform);

    FORCEINLINE_DEBUGGABLE bool IsFinsihed()
    {
        return !GetPaendingCookAssetsSet().Num();
    }
    
    FORCEINLINE_DEBUGGABLE TMap<FName,FName>& GetAssetTypeMapping(){ return AssetTypeMapping; }
    void OnAsyncObjectLoaded(FSoftObjectPath ObjectPath,const TArray<ITargetPlatform*>& Platforms,FCookActionCallback CookActionCallback);

    void WaitCookerFinished();
    
private:
#if WITH_PACKAGE_CONTEXT
    FORCEINLINE TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>>& GetPlatformSavePackageContexts() {return PlatformSavePackageContexts;}
    TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContextsRaw();
    TMap<FString, FSavePackageContext*> GetPlatformSavePackageContextsNameMapping();
#endif
    
private:
    TArray<FCookCluster> CookClusters;
    FCookerFailedCollection CookFailedAssetsCollection;
    TSharedPtr<FPackageTracker> PackageTracker;
    TSharedPtr<FThreadWorker> WaitThreadWorker;
    TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
    TSharedPtr<struct FCookShaderCollectionProxy> PlatformCookShaderCollection;
    FPackagePathSet PackagePathSet;
    TMap<ETargetPlatform,TArray<FName>> CookAssetOrders;

private:
    // async
    TMap<FName,FName> AssetTypeMapping;
    TSet<FName>& GetPaendingCookAssetsSet(){ return PaendingCookAssetsSet; }
    TSet<FName> PaendingCookAssetsSet;
};