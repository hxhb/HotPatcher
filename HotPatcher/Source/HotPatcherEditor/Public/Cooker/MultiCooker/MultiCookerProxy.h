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


UCLASS()
class HOTPATCHEREDITOR_API UMultiCookerProxy:public UHotPatcherProxyBase
{
    GENERATED_BODY()
public:
    virtual void Init()override;
    virtual void Shutdown() override;
    virtual bool DoExport()override;
    virtual FMultiCookerSettings* GetSettingObject()override {return (FMultiCookerSettings*)(Setting);};
protected:
    FExportPatchSettings MakePatchSettings();
protected:
    void OnOutputMsg(const FString& InMsg);
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
};