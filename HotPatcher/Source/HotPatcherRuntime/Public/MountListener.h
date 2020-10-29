// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "MountListener.generated.h"

USTRUCT(Blueprintable,BlueprintType)
struct HOTPATCHERRUNTIME_API FPakMountInfo
{
    GENERATED_USTRUCT_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    FString Pak;
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    int32 PakOrder;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnListenerMountPak,FPakMountInfo,PakInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnListenerUnMountPak,const FString&,PakName);
/**
* 
*/
UCLASS(Blueprintable,BlueprintType)
class HOTPATCHERRUNTIME_API UMountListener : public UObject
{
    GENERATED_UCLASS_BODY()
public:

    UFUNCTION(BlueprintCallable)
    void Init();
#if ENGINE_MINOR_VERSION >24
    void OnMountPak(const TCHAR* Pak, int32 ChunkID);
#else
    virtual void OnMountPak(const TCHAR* Pak);
#endif
    virtual bool OnUnMountPak(const FString& Pak);

    virtual TMap<FString,FPakMountInfo>& GetMountedPaks();

    UPROPERTY(BlueprintAssignable)
    FOnListenerMountPak OnMountPakDelegate;
    UPROPERTY(BlueprintAssignable)
    FOnListenerUnMountPak OnUnMountPakDelegate;
    
    private:
    TMap<FString,FPakMountInfo> PaksMap;
};