// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "HotPatcherRuntime.h"
#include "FlibPatchParserHelper.h"
#include "ETargetPlatform.h"
#include "Interfaces/ITargetPlatform.h"
#include "Misc/EnumRange.h"

#if WITH_EDITOR
#include "Interfaces/ITargetPlatformManagerModule.h"
#endif

#define LOCTEXT_NAMESPACE "FHotPatcherRuntimeModule"


void FHotPatcherRuntimeModule::StartupModule()
{
	TArray<FString> AppendPlatformEnums;
	
#if WITH_EDITOR
	TArray<FString> RealPlatformEnums;
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();
	for (ITargetPlatform *TargetPlatformIns : TargetPlatforms)
	{
		FString PlatformName = TargetPlatformIns->PlatformName();
		if(!PlatformName.IsEmpty())
		{
			RealPlatformEnums.AddUnique(PlatformName);
		}
	}
	AppendPlatformEnums = RealPlatformEnums;
#endif
	
	TArray<TPair<FName, int64>> EnumNames = THotPatcherTemplateHelper::AppendEnumeraters<ETargetPlatform>(AppendPlatformEnums);
}

void FHotPatcherRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherRuntimeModule, HotPatcherRuntime)