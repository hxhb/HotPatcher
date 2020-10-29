// Fill out your copyright notice in the Description page of Project Settings.

#include "MountListener.h"
#include "FlibPakHelper.h"
#include "IPlatformFilePak.h"

DECLARE_LOG_CATEGORY_CLASS(LogMountListener, Log, All);

UMountListener::UMountListener(const FObjectInitializer& Initializer):Super(Initializer)
{
}

void UMountListener::Init()
{
    if(!HasAnyFlags(RF_ClassDefaultObject))
    {
#if ENGINE_MINOR_VERSION > 24
        FCoreDelegates::OnPakFileMounted.AddUObject(this,&UMountListener::OnMountPak);
#else
        FCoreDelegates::PakFileMountedCallback.AddUObject(this,&UMountListener::OnMountPak);
#endif
        FCoreDelegates::OnUnmountPak.BindUObject(this,&UMountListener::OnUnMountPak);
#if !WITH_EDITOR
        FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));
        TArray<FString> MountedPaks = UFlibPakHelper::GetAllMountedPaks();
        for(const auto& Pak:MountedPaks)
        {
#if ENGINE_MINOR_VERSION > 24
            OnMountPak(*Pak,UFlibPakHelper::GetPakOrderByPakPath(Pak));
#else
            OnMountPak(*Pak);
#endif
        }
#endif
    }
}
#if ENGINE_MINOR_VERSION > 24
void UMountListener::OnMountPak(const TCHAR* Pak, int32 ChunkID)
#else
void UMountListener::OnMountPak(const TCHAR* Pak)
#endif
{
    UE_LOG(LogMountListener,Log,TEXT("Pak %s is Mounted!"),Pak);
    FPakMountInfo MountedPak;
    MountedPak.Pak = Pak;
#if ENGINE_MINOR_VERSION > 24
    MountedPak.PakOrder = ChunkID;
#else
    MountedPak.PakOrder = UFlibPakHelper::GetPakOrderByPakPath(Pak);
#endif
    PaksMap.Add(MountedPak.Pak,MountedPak);
    OnMountPakDelegate.Broadcast(MountedPak);
}

bool UMountListener::OnUnMountPak(const FString& Pak)
{
    PaksMap.Remove(Pak);
    OnUnMountPakDelegate.Broadcast(Pak);
    return true;
}

TMap<FString, FPakMountInfo>& UMountListener::GetMountedPaks()
{
    return PaksMap;
}