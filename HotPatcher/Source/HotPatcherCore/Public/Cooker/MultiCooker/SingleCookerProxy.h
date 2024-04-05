#pragma once
#include "FSingleCookerSettings.h"
#include "Cooker/HotPatcherCookerSettingBase.h"
#include "CreatePatch/HotPatcherProxyBase.h"
#include "HotPatcherBaseTypes.h"
#include "BaseTypes/FPackageTracker.h"
// engine header
#include "Templates/SharedPointer.h"
#include "Containers/Queue.h"
#include "TickableEditorObject.h"
#include "ThreadUtils/FThreadUtils.hpp"
#include "SingleCookerProxy.generated.h"

USTRUCT()
struct FCookerPreCacheDDC
{
    GENERATED_BODY()
    
    UPROPERTY()
    TArray<UPackage*> Packages;
    UPROPERTY()
    TSet<UObject*> ProcessedObjects;
    UPROPERTY()
    TSet<UObject*> PendingCachePlatformDataObjects;

    void Empty()
    {
        Packages.Empty();
        ProcessedObjects.Empty();
        ProcessedObjects.Empty();
    }
};

bool IsAlwayPostLoadClasses(UPackage* Package, UObject* Object);

struct FFreezePackageTracker : public FPackageTrackerBase
{
public:
    FFreezePackageTracker(const TSet<FName>& InCookerAssetsSet,const TSet<FName>& InAllAssets):CookerAssetsSet(InCookerAssetsSet),AllAssetsSet(InAllAssets){}

    virtual ~FFreezePackageTracker(){}
	
    virtual void NotifyUObjectCreated(const class UObjectBase *Object, int32 Index) override;
    virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override
    {
        auto ObjectOuter = const_cast<UObject*>(static_cast<const UObject*>(Object));
        if(FreezeObjects.Contains(ObjectOuter))
        {
            FreezeObjects.Remove(ObjectOuter);
        }
    }
    bool IsFreezed(UObject* Object)const
    {
        return FreezeObjects.Contains(Object);        
    }
protected:
    TSet<FName> CookerAssetsSet;
    TSet<FName> AllAssetsSet;
    TSet<UObject*> FreezeObjects;
    TMultiMap<FName,TArray<UObject*>> PackageObjectsMap;
    // TArray<UObject*> FreezeObjects;
};

struct FOtherCookerPackageTracker : public FPackageTrackerBase
{
    FOtherCookerPackageTracker(const TSet<FName>& InCookerAssets,const TSet<FName>& InAllCookerAssets):CookerAssets(InCookerAssets),AllCookerAssets(InAllCookerAssets){};
	
    virtual ~FOtherCookerPackageTracker(){}
    FORCEINLINE virtual void OnPackageCreated(UPackage* Package) override
    {
        FName AssetPathName = FName(*Package->GetPathName());
        if(!CookerAssets.Contains(AssetPathName) && AllCookerAssets.Contains(AssetPathName))
        {
            LoadOtherCookerAssets.Add(AssetPathName);
            UE_LOG(LogHotPatcher,Display,TEXT("[OtherCookerPackageTracker] %s "),*AssetPathName.ToString());
        }
    }
    FORCEINLINE const TSet<FName>& GetLoadOtherCookerAssets()const { return LoadOtherCookerAssets; }
protected:
    TSet<FName> CookerAssets;
    TSet<FName> AllCookerAssets;
    TSet<FName> LoadOtherCookerAssets;
};

DECLARE_MULTICAST_DELEGATE(FSingleCookerEvent);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSingleCookActionEvent,const FSoftObjectPath&,ETargetPlatform);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FSingleCookResultEvent,const FSoftObjectPath&,ETargetPlatform,ESavePackageResult);


UENUM()
enum class ECookClusterType:uint8
{
    Normal,
    Accompany
};

USTRUCT()
struct FCookCluster
{
    GENERATED_BODY()
    UPROPERTY()
    TArray<FAssetDetail> AssetDetails;
    UPROPERTY()
    TSet<FName> AssetTypes;
    
    UPROPERTY()
    TArray<ETargetPlatform> Platforms;
    UPROPERTY()
    bool bPreGeneratePlatformData = false;
    UPROPERTY()
    bool bCacheDDCOnly = false;

    UPROPERTY()
    ECookClusterType ClusterType;
public: // for advanced cook
    UPROPERTY()
    bool bShaderCluster = false;

    FORCEINLINE_DEBUGGABLE TArray<FSoftObjectPath> AsSoftObjectPaths()const
    {
        TArray<FSoftObjectPath> SoftObjectPaths;
        for(const auto& AssetDetail:AssetDetails)
        {
            if(AssetDetail.IsValid())
            {
                SoftObjectPaths.Emplace(AssetDetail.PackagePath.ToString());
            }
        }
        return SoftObjectPaths;
    }
    FCookActionCallback CookActionCallback;
};


struct FCookClusterPack
{
    virtual ~FCookClusterPack(){}
    FCookClusterPack()=default;
    
    ECookClusterType Type;
    bool Enqueue(const FCookCluster& CookCluster)
    {
        ++TotalClusterCount;
        return CluserQueue.Enqueue(CookCluster);
    }
    bool Dequeue(FCookCluster& CookCluster)
    {
        ++ExectedCount;
        return CluserQueue.Dequeue(CookCluster);
    }
    bool IsEmpty(){ return CluserQueue.IsEmpty(); }
    TSharedPtr<FCookCluster>& GetExecutingCluserRef(){ return ExecutingCluser; }

    enum EClusterCountType
    {
        Total,
        Executed
    };
    int32 GetClusterCount(EClusterCountType InType)const
    {
        return InType == EClusterCountType::Total ? TotalClusterCount : ExectedCount;
    }
private:
    TQueue<FCookCluster> CluserQueue;
    TSharedPtr<FCookCluster> ExecutingCluser = nullptr;
    int32 TotalClusterCount = 0;
    int32 ExectedCount = 0;
};

UCLASS()
class HOTPATCHERCORE_API USingleCookerProxy:public UHotPatcherProxyBase, public FTickableEditorObject
{
    GENERATED_UCLASS_BODY()
public:
    virtual void Init(FPatcherEntitySettingBase* InSetting)override;
    virtual void Shutdown() override;
    
    virtual bool DoExport()override;

    void ExecuteNextCluster();
    
    bool IsFinsihed();
    bool HasError();
    virtual FSingleCookerSettings* GetSettingObject()override {return (FSingleCookerSettings*)(Setting);};

public: // base func
    virtual void Tick(float DeltaTime) override;
    TStatId GetStatId() const override;
    bool IsTickable() const override;
    
public: // core interface
    int32 MakeCookQueue(FCookCluster& InCluser);
    void PreGeneratePlatformData(const FCookCluster& CookCluster,bool bWaitFinished = true);
    void CleanClusterCachedPlatformData(const FCookCluster& CookCluster);
    void ExecCookCluster(const FCookCluster& Cluster,bool bWaitFinished = true);
    FCookerFailedCollection& GetCookFailedAssetsCollection(){return CookFailedAssetsCollection;};
    void CleanOldCooked(const FString& CookBaseDir,const TArray<FSoftObjectPath>& ObjectPaths,const TArray<ETargetPlatform>& CookPlatforms);
    // cook classes order
    TArray<UClass*> GetPreCacheClasses()const;

public: // callback
    FCookActionResultEvent GetOnPackageSavedCallback();
    FCookActionEvent GetOnCookAssetBeginCallback();
    
public: // packae tracker interface
    TSet<FName> GetCookerAssets();
    TSet<FName> GetAdditionalAssets();
    /*
     * 获取PackageTracker追踪到的资源
     * bPadding：为true则获取GetPendingPackageSet，为false则获取所有追踪到的资源GetAdditionalPackageSet
     */
    FCookCluster GetPackageTrackerAsCluster(bool bPadding);
    TArray<FName>& GetPlatformCookAssetOrders(ETargetPlatform Platform);
    bool HasValidPackageTracker();
protected: // on asset cooked call func
    void OnAssetCookedHandle(const FSoftObjectPath& PackagePath,ETargetPlatform Platform,ESavePackageResult Result);

protected: // metadata func
    void BulkDataManifest();
    void IoStoreManifest();
    void InitShaderLibConllections();
    void ShutdowShaderLibCollections();
    
private: // package context
#if WITH_PACKAGE_CONTEXT
    FORCEINLINE TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>>& GetPlatformSavePackageContexts() {return PlatformSavePackageContexts;}
    TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContextsRaw();
    TMap<FString, FSavePackageContext*> GetPlatformSavePackageContextsNameMapping();
    TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
#endif

public: // static function
    static void DumpCluster(const FCookCluster& CookCluster, bool bWriteToLog);

public:
    int32 GetClassAssetNumOfPerCluster(UClass* Class);
    
public: // delegate
    FSingleCookerEvent OnCookBegin;
    FSingleCookerEvent OnCookFinished;
    FSingleCookActionEvent OnCookAssetBegin;
    FSingleCookResultEvent OnAssetCooked;

private: // cook assets manager
    FCookerFailedCollection CookFailedAssetsCollection;
    TMap<ECookClusterType,TSharedPtr<FCookClusterPack>> CookClusterPackMap;
    FCookClusterPack& GetClusterPackByType(ECookClusterType Type)
    {
        if(!CookClusterPackMap.Contains(Type) || !CookClusterPackMap.Find(Type)->IsValid())
        {
            CookClusterPackMap.Add(Type,MakeShareable(new FCookClusterPack));
        }
        return **CookClusterPackMap.Find(Type);
    }
    
    int32 GetClusterCount(FCookClusterPack::EClusterCountType CountType);

    UPROPERTY()
    FCookerPreCacheDDC CookerPreCacheDDC;
private: // metadate
    TSharedPtr<struct FCookShaderCollectionProxy> PlatformCookShaderCollection;
    TMap<ETargetPlatform,TArray<FName>> CookAssetOrders;
    
private: // package tracker
    TSharedPtr<FPackageTracker> PackageTracker;
    TSharedPtr<FOtherCookerPackageTracker> OtherCookerPackageTracker;
    TSharedPtr<FFreezePackageTracker> FreezePackageTracker;
    
    FPackagePathSet ExixtPackagePathSet;

private: // worker flow control
    /** Critical section for synchronizing access to sink. */
    mutable FCriticalSection SynchronizationObject;
    TSharedPtr<FThreadWorker> WaitThreadWorker;
    bool IsAlready()const{ return bAlready; }
    bool bAlready = false;
};
