#pragma once
// project header
#include "CreatePatch/HotPatcherSettingBase.h"
#include "HotPatcherLog.h"
#include "CreatePatch/HotPatcherContext.h"
#include "CreatePatch/TimeRecorder.h"
#include "HotPatcherDelegates.h"

// engine header
#include "CoreMinimal.h"
#include "HotPatcherProxyBase.generated.h"



UCLASS()
class HOTPATCHERCORE_API UHotPatcherProxyBase : public UObject
{
public:
    GENERATED_BODY()


    virtual void Init(FPatcherEntitySettingBase* InSetting);
    virtual void Shutdown();
    FORCEINLINE virtual bool DoExport(){return false;};
    FORCEINLINE virtual FPatcherEntitySettingBase* GetSettingObject(){return Setting;};
    IAssetRegistry* GetAssetRegistry()const { return AssetRegistry; }
protected:
    FORCEINLINE virtual void SetProxySettings(FPatcherEntitySettingBase* InSetting)
    {
        Setting = InSetting;
    }
    const TMap<ETargetPlatform,FName>& GetPlatformNameMapping(){ return PlatformNameMapping; }
public:
#if WITH_PACKAGE_CONTEXT
    // virtual void InitPlatformPackageContexts();
    FORCEINLINE TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> GetPlatformSavePackageContexts()const {return PlatformSavePackageContexts;}
    FORCEINLINE TMap<ETargetPlatform,FSavePackageContext*> GetPlatformSavePackageContextsRaw()const;
    TMap<ETargetPlatform,TSharedPtr<FSavePackageContext>> PlatformSavePackageContexts;
#endif

public:
    FExportPakProcess OnPaking;
    FExportPakShowMsg OnShowMsg;
protected:
    FPatcherEntitySettingBase* Setting;
    IAssetRegistry* AssetRegistry = NULL;
    TMap<ETargetPlatform,FName> PlatformNameMapping;
};

inline void UHotPatcherProxyBase::Init(FPatcherEntitySettingBase* InSetting)
{
    SetProxySettings(InSetting);
    
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    AssetRegistry = &AssetRegistryModule.Get();

    UEnum* UEnumIns = THotPatcherTemplateHelper::GetUEnum<ETargetPlatform>();
    for (int64 EnumIndex = 0;EnumIndex < UEnumIns->GetMaxEnumValue();++EnumIndex)
    {
        if(UEnumIns->IsValidEnumValue(EnumIndex))
        {
            FName EnumtorName = UEnumIns->GetNameByValue(EnumIndex);
            PlatformNameMapping.Add((ETargetPlatform)EnumIndex,EnumtorName);
        }
    }
}

inline void UHotPatcherProxyBase::Shutdown()
{
    AssetRegistry = nullptr;
}


#if WITH_PACKAGE_CONTEXT
FORCEINLINE TMap<ETargetPlatform, FSavePackageContext*> UHotPatcherProxyBase::GetPlatformSavePackageContextsRaw() const
{
    TMap<ETargetPlatform,FSavePackageContext*> result;
    TArray<ETargetPlatform> Keys;
    GetPlatformSavePackageContexts().GetKeys(Keys);
    for(const auto& Key:Keys)
    {
        result.Add(Key,GetPlatformSavePackageContexts().Find(Key)->Get());
    }
    return result;
}
#endif
