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
DECLARE_MULTICAST_DELEGATE_TwoParams(FSingleCookActionEvent,const FSoftObjectPath&,ETargetPlatform);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FSingleCookResultEvent,const FSoftObjectPath&,ETargetPlatform,ESavePackageResult);

USTRUCT()
struct FCookCluster
{
    GENERATED_BODY()
    UPROPERTY()
    TArray<FAssetDetail> AssetDetails;
    UPROPERTY()
    TArray<ETargetPlatform> Platforms;
    UPROPERTY()
    bool bPreGeneratePlatformData = false;


    FORCEINLINE_DEBUGGABLE TArray<FSoftObjectPath> AsSoftObjectPaths()const
    {
        TArray<FSoftObjectPath> SoftObjectPaths;
        for(const auto& AssetDetail:AssetDetails)
        {
            SoftObjectPaths.Emplace(AssetDetail.PackagePath);
        }
        return SoftObjectPaths;
    }
    FCookActionCallback CookActionCallback;
};


UCLASS()
class HOTPATCHERCORE_API USingleCookerProxy:public UHotPatcherProxyBase
{
    GENERATED_BODY()
public:
    virtual void Init(FPatcherEntitySettingBase* InSetting)override;
    virtual void Shutdown() override;
    virtual bool DoExport()override;
    bool IsFinsihed();
    bool HasError();
    virtual FSingleCookerSettings* GetSettingObject()override {return (FSingleCookerSettings*)(Setting);};
    FCookerFailedCollection& GetCookFailedAssetsCollection(){return CookFailedAssetsCollection;};

    void CookClusterSync(const FCookCluster& CookCluster);
    void CookCluster(const FCookCluster& CookCluster);
    void AddCluster(const FCookCluster& CookCluster);
    
    TArray<FName>& GetPlatformCookAssetOrders(ETargetPlatform Platform);
    TSet<FName> GetAdditionalAssets();
    TArray<UClass*> GetPreCacheClasses()const;
protected:
    void OnAssetCookedHandle(const FSoftObjectPath& PackagePath,ETargetPlatform Platform,ESavePackageResult Result);
    void DoCookMission(const TArray<FAssetDetail>& Assets);
    void BulkDataManifest();
    void IoStoreManifest();
    
    void InitShaderLibConllections();
    void ShutdowShaderLibCollections();

    void MarkAssetCooked(const FSoftObjectPath& PackagePath,ETargetPlatform Platform);

    void PreGeneratePlatformData(const FCookCluster& CookCluster);
    void WaitCookerFinished();
    
private:
#if WITH_PACKAGE_CONTEXT
    FORCEINLINE TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>>& GetPlatformSavePackageContexts() {return PlatformSavePackageContexts;}
    TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContextsRaw();
    TMap<FString, FSavePackageContext*> GetPlatformSavePackageContextsNameMapping();
#endif

public:
    FSingleCookerEvent OnCookBegin;
    FSingleCookerEvent OnCookFinished;
    FSingleCookActionEvent OnCookAssetBegin;
    FSingleCookResultEvent OnAssetCooked;
    
private:
    FORCEINLINE_DEBUGGABLE TMap<FName,FName>& GetAssetTypeMapping(){ return AssetTypeMapping; }
    FORCEINLINE_DEBUGGABLE TSet<FName>& GetPaendingCookAssetsSet(){ return PaendingCookAssetsSet; }
    
    TMap<FName,FName> AssetTypeMapping;
    TSet<FName> PaendingCookAssetsSet;

    /** Critical section for synchronizing access to sink. */
    mutable FCriticalSection SynchronizationObject;

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