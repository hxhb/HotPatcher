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
    UScopedSlowTaskContext* UnrealPakSlowTask;
    TSharedPtr<TimeRecorder> TotalTimeRecorder;
};



USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FHotPatcherPatchContext:public FHotPatcherContext
{
    GENERATED_USTRUCT_BODY()
    FHotPatcherPatchContext()=default;
    virtual FExportPatchSettings* GetSettingObject(){ return (FExportPatchSettings*)ContextSetting; }
    
    virtual FString GetTotalTimeRecorderName()const{return TEXT("Generate the patch total time");}

    FORCEINLINE FPatchVersionExternDiff* GetPatcherDiffInfoByName(const FString& PlatformName);
    FORCEINLINE FPlatformExternAssets* GetPatcherChunkInfoByName(const FString& PlatformName,const FString& ChunkName);
    
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
        PakFilesInfoMap.GetKeys(Keys);
        return Keys.Num();
    }
    TMap<FString,TArray<FPakFileInfo>> PakFilesInfoMap;


};

FPatchVersionExternDiff* FHotPatcherPatchContext::GetPatcherDiffInfoByName(const FString& PlatformName)
{
    ETargetPlatform Platform;
    UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
    // add new file to diff
    FPatchVersionExternDiff* PatchVersionExternDiff = NULL;
    {
        if(!VersionDiff.PlatformExternDiffInfo.Contains(Platform))
        {
            FPatchVersionExternDiff AdditionalFiles;
            VersionDiff.PlatformExternDiffInfo.Add(Platform,AdditionalFiles);
        }
        PatchVersionExternDiff = VersionDiff.PlatformExternDiffInfo.Find(Platform);
    }
    return PatchVersionExternDiff;
};
FPlatformExternAssets* FHotPatcherPatchContext::GetPatcherChunkInfoByName(const FString& PlatformName,const FString& ChunkName)
{
    ETargetPlatform Platform;
    UFlibPatchParserHelper::GetEnumValueByName(PlatformName,Platform);
    FPlatformExternAssets* PlatformExternAssetsPtr = NULL;
    {
        for(auto& PakChunk:PakChunks)
        {
            if(PakChunk.ChunkName.Equals(ChunkName))
            {
                for(auto& PlatfromExternAssetsRef:PakChunk.AddExternAssetsToPlatform)
                {
                    if(PlatfromExternAssetsRef.TargetPlatform == Platform)
                    {
                        PlatformExternAssetsPtr = &PlatfromExternAssetsRef;
                    }
                }
                if(!PlatformExternAssetsPtr)
                {
                    FPlatformExternAssets PlatformExternAssets;
                    PlatformExternAssets.TargetPlatform = Platform;
                    PlatformExternAssetsPtr = &(PakChunk.AddExternAssetsToPlatform.Add_GetRef(PlatformExternAssets));
                }
            }
        }
        return PlatformExternAssetsPtr;
    }
}

USTRUCT(BlueprintType)
struct HOTPATCHERRUNTIME_API FHotPatcherReleaseContext:public FHotPatcherContext
{
    GENERATED_USTRUCT_BODY()
    FHotPatcherReleaseContext()=default;
    virtual FExportReleaseSettings* GetSettingObject() { return (FExportReleaseSettings*)ContextSetting; }
    virtual FString GetTotalTimeRecorderName()const{return TEXT("Generate the release total time");}
    UPROPERTY(BlueprintReadOnly)
    FHotPatcherVersion NewReleaseVersion;
    
};