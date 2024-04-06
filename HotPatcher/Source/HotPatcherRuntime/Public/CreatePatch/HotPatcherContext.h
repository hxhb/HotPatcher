#pragma once

#include "HotPatcherSettingBase.h"
#include "FPatchVersionDiff.h"
#include "FHotPatcherVersion.h"
#include "FChunkInfo.h"
#include "FPakFileInfo.h"
#include "FExportReleaseSettings.h"
#include "HotPatcherSettingBase.h"
#include "Misc/ScopedSlowTask.h"
#include "CreatePatch/ScopedSlowTaskContext.h"
#include "BaseTypes/FPackageTracker.h"

// engine
#include "CoreMinimal.h"
#include "TimeRecorder.h"
#include "Engine/EngineTypes.h"
#include "HotPatcherContext.generated.h"

struct FChunkInfo;
struct FHotPatcherVersion;


DECLARE_MULTICAST_DELEGATE_TwoParams(FExportPakProcess,const FString&,const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FExportPakShowMsg,const FString&);


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FHotPatcherContext
{
    GENERATED_USTRUCT_BODY()
    FHotPatcherContext()=default;
    virtual ~FHotPatcherContext(){}
    virtual void Init()
    {
        TotalTimeRecorder = MakeShareable(new TimeRecorder(GetTotalTimeRecorderName(),false));
        TotalTimeRecorder->Begin(GetTotalTimeRecorderName());
    }
    virtual void Shurdown()
    {
        TotalTimeRecorder->End();
    }
    virtual FString GetTotalTimeRecorderName()const{return TEXT("");}
    //virtual FHotPatcherSettingBase* GetSettingObject() { return (FHotPatcherSettingBase*)ContextSetting; }

public:
    FExportPakProcess OnPaking;
    FExportPakShowMsg OnShowMsg;
    FHotPatcherSettingBase* ContextSetting;
    UPROPERTY()
    UScopedSlowTaskContext* UnrealPakSlowTask = nullptr;
    TSharedPtr<TimeRecorder> TotalTimeRecorder;
};

struct HOTPATCHERRUNTIME_API FPackageTrackerByDiff : public FPackageTrackerBase
{
    FPackageTrackerByDiff(struct FHotPatcherPatchContext& InContext):Context(InContext)
    {}

    virtual ~FPackageTrackerByDiff(){}

    virtual void OnPackageCreated(UPackage* Package) override;
    virtual void OnPackageDeleted(UPackage* Package) override{}

    const TMap<FName,FAssetDetail>& GetTrackResult()const { return TrackedAssets; }	
protected:
    TMap<FName,FAssetDetail> TrackedAssets;
    FHotPatcherPatchContext& Context;
};


USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FHotPatcherPatchContext:public FHotPatcherContext
{
    GENERATED_USTRUCT_BODY()
    FHotPatcherPatchContext()=default;

    virtual void Init() override;
    virtual FExportPatchSettings* GetSettingObject(){ return (FExportPatchSettings*)ContextSetting; }
    virtual FString GetTotalTimeRecorderName()const{return TEXT("Generate the patch total time");}

    FPatchVersionExternDiff* GetPatcherDiffInfoByName(const FString& PlatformName);
    FPlatformExternAssets* GetPatcherChunkInfoByName(const FString& PlatformName,const FString& ChunkName);
    
    // UPROPERTY(EditAnywhere)
    class UPatcherProxy* PatchProxy;
    
    // base version content
    UPROPERTY(EditAnywhere)
    FHotPatcherVersion BaseVersion;
    // current project content
    UPROPERTY(EditAnywhere)
    FHotPatcherVersion CurrentVersion;

    // base version and current version different
    UPROPERTY(EditAnywhere)
    FPatchVersionDiff VersionDiff;

    // final new version content
    UPROPERTY(EditAnywhere)
    FHotPatcherVersion NewReleaseVersion;

    // generated current version chunk
    UPROPERTY(EditAnywhere)
    FChunkInfo NewVersionChunk;
    
    // chunk info
    UPROPERTY(EditAnywhere)
    TArray<FChunkInfo> PakChunks;

    UPROPERTY(EditAnywhere)
    TArray<FPakCommand> AdditionalFileToPak;
    
    // every pak file info
    // TArray<FPakFileProxy> PakFileProxys;
    FORCEINLINE uint32 GetPakFileNum()const 
    {
        TArray<FString> Keys;
        PakFilesInfoMap.PakFilesMap.GetKeys(Keys);
        return Keys.Num();
    }
    FPakFilesMap PakFilesInfoMap;
    bool AddAsset(const FString ChunkName, const FAssetDetail& AssetDetail);
    
    void AddExternalFile(const FString& PlatformName,const FString& ChunkName,const FExternFileInfo& AddFile)
    {
        if(!IsContainInBase(PlatformName,AddFile))
        {
            GetPatcherDiffInfoByName(PlatformName)->AddExternalFiles.Add(AddFile);
            GetPatcherChunkInfoByName(PlatformName,ChunkName)->AddExternFileToPak.Add(AddFile); 
        }
    }
    bool IsContainInBase(const FString& PlatformName,const FExternFileInfo& AddFile);
};

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FHotPatcherReleaseContext:public FHotPatcherContext
{
    GENERATED_USTRUCT_BODY()
    FHotPatcherReleaseContext()=default;
    virtual FExportReleaseSettings* GetSettingObject() { return (FExportReleaseSettings*)ContextSetting; }
    virtual FString GetTotalTimeRecorderName()const{return TEXT("Generate the release total time");}
    UPROPERTY(BlueprintReadOnly)
    FHotPatcherVersion NewReleaseVersion;
    class UReleaseProxy* ReleaseProxy;
};