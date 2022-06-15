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
#if ENGINE_MAJOR_VERSION >4 || ENGINE_MINOR_VERSION >=26
    	FCoreDelegates::OnPakFileMounted2.AddLambda([this](const IPakFile& PakFile){this->OnMountPak(*PakFile.PakGetPakFilename(),0);});
#endif

#if ENGINE_MINOR_VERSION <=25 && ENGINE_MINOR_VERSION > 24
        FCoreDelegates::OnPakFileMounted.AddLambda([this](const TCHAR* Pak, const int32 ChunkID){this->OnMountPak(Pak,ChunkID);});
#endif
    	
#if ENGINE_MAJOR_VERSION <=4 && ENGINE_MINOR_VERSION <= 24
    	FCoreDelegates::PakFileMountedCallback.AddLambda([this](const TCHAR* Pak){this->OnMountPak(Pak,0);});
#endif
        FCoreDelegates::OnUnmountPak.BindUObject(this,&UMountListener::OnUnMountPak);
#if !WITH_EDITOR
        FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));
        TArray<FString> MountedPaks = UFlibPakHelper::GetAllMountedPaks();
        for(const auto& Pak:MountedPaks)
        {
#if ENGINE_MAJOR_VERSION >4 || ENGINE_MINOR_VERSION > 24
            OnMountPak(*Pak,UFlibPakHelper::GetPakOrderByPakPath(Pak));
#else
            OnMountPak(*Pak);
#endif
        }
#endif
    }
}

void UMountListener::OnMountPak(const TCHAR* PakFileName, int32 ChunkID)
{
    UE_LOG(LogMountListener,Log,TEXT("Pak %s is Mounted!"),PakFileName);
    FPakMountInfo MountedPak;
    MountedPak.Pak = PakFileName;
    MountedPak.PakOrder = ChunkID;
    MountedPak.PakOrder = UFlibPakHelper::GetPakOrderByPakPath(PakFileName);
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