#include "TargetPlatformRegister.h"

#include "ETargetPlatform.h"
#include "HotPatcherTemplateHelper.hpp"
#include "HotPatcherLog.h"

#if WITH_EDITOR
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#endif

UTargetPlatformRegister::UTargetPlatformRegister(const FObjectInitializer& Initializer):Super(Initializer)
{
	TArray<FString> AppendPlatformEnums;
    	
#if WITH_EDITOR
	TArray<FString> RealPlatformEnums;
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	UE_LOG(LogHotPatcher,Display,TEXT("TargetPlatformRegister:"));
	for (ITargetPlatform *TargetPlatformIns : TargetPlatforms)
	{
		FString PlatformName = TargetPlatformIns->PlatformName();
		if(!PlatformName.IsEmpty())
		{
			UE_LOG(LogHotPatcher,Display,TEXT("\t%s"),*PlatformName);
			RealPlatformEnums.AddUnique(PlatformName);
		}
	}
	AppendPlatformEnums = RealPlatformEnums;
#endif
	
	TArray<TPair<FName, int64>> EnumNames = THotPatcherTemplateHelper::AppendEnumeraters<ETargetPlatform>(AppendPlatformEnums);
}