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
        FCoreDelegates::OnPakFileMounted.AddUObject(this,&UMountListener::OnMountPak);
        FCoreDelegates::OnUnmountPak.BindUObject(this,&UMountListener::OnUnMountPak);
    #if !WITH_EDITOR
        FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));
        TArray<FString> MountedPaks = UFlibPakHelper::GetAllMountedPaks();
        for(const auto& Pak:MountedPaks)
        {
            OnMountPak(*Pak,UFlibPakHelper::GetPakOrderByPakPath(Pak));
        }
    #endif
    }
}

void UMountListener::OnMountPak(const TCHAR* Pak, int32 ChunkID)
{
    UE_LOG(LogMountListener,Log,TEXT("Pak %s is Mounted! PakOrder as %d"),Pak,ChunkID);
    FPakMountInfo MountedPak;
    MountedPak.Pak = Pak;
    MountedPak.PakOrder = ChunkID;

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
