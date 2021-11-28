#pragma once
#include "CreatePatch/HotPatcherProxyBase.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "FMultiCookerSettings.h"

// engine header
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#include "MultiCookerProxy.generated.h"

class UMultiCookerProxy;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMultiCookerStatusChanged,UMultiCookerProxy*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSingleCookerFinished,bool,const FAssetsCollection&);

UCLASS()
class HOTPATCHEREDITOR_API UMultiCookerProxy:public UHotPatcherProxyBase
{
    GENERATED_BODY()
public:
    virtual void Init()override;
    virtual void Shutdown() override;
    virtual bool DoExport()override;
    virtual FMultiCookerSettings* GetSettingObject()override {return (FMultiCookerSettings*)(Setting);};
public:
    FOnMultiCookerStatusChanged OnMultiCookerBegining;
    FOnMultiCookerStatusChanged OnMultiCookerFinished;
    FOnSingleCookerFinished OnSingleCookerFinished;
    bool IsRunning()const;
    void Cancel();
    bool HasError();
    void OnCookMissionsFinished(bool bSuccessed);
    bool MergeShader();
protected:
    FExportPatchSettings MakePatchSettings();
    TArray<FSingleCookerSettings> MakeSingleCookerSettings(const TArray<FAssetDetail>& AllDetails);
protected:
    void UpdateMultiCookerStatus();
    void UpdateSingleCookerStatus(bool bSuccessed,const FAssetsCollection& FailedCollection);
    void OnOutputMsg(FProcWorkerThread* Worker,const FString& InMsg);
    void OnCookProcBegin(FProcWorkerThread* ProcWorker);
    void OnCookProcSuccessed(FProcWorkerThread* ProcWorker);
    void OnCookProcFailed(FProcWorkerThread* ProcWorker);
protected:
    TMap<FString,FAssetDependenciesInfo>& GetAssetsDependenciesScanedCaches(){ return ScanedCaches; };
    TSharedPtr<FProcWorkerThread> CreateProcMissionThread(const FString& Bin, const FString& Command, const FString& MissionName);
    TSharedPtr<FProcWorkerThread> CreateSingleCookWroker(const FSingleCookerSettings& SingleCookerSettings);
    
private:
    TSharedPtr<FMultiCookerSettings> MultiCookerSettings;
    TMap<FString,FAssetDependenciesInfo> ScanedCaches;
    TMap<FString,TSharedPtr<FProcWorkerThread>> CookerProcessMap;
    TMap<FString,FSingleCookerSettings> CookerConfigMap;
    TMap<FString,FAssetsCollection> CookerFailedCollectionMap;
    int32 FinishedCount = 0;
};