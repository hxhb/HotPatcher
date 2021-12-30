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
	UEnum* TargetPlatform = THotPatcherTemplateHelper::GetUEnum<ETargetPlatform>();
	uint64 MaxEnumValue = TargetPlatform->GetMaxEnumValue()-2;
	FString EnumName = TargetPlatform->GetName();
	TArray<TPair<FName, int64>> EnumNames;
	
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
	for (ETargetPlatform Platform:TEnumRange<ETargetPlatform>())
	{
		FName EnumtorName = TargetPlatform->GetNameByValue((int64)Platform);
		EnumNames.Emplace(EnumtorName,(int64)Platform);
		AppendPlatformEnums.Remove(EnumtorName.ToString());
	}
	for(const auto& AppendEnumItem:AppendPlatformEnums)
	{
		++MaxEnumValue;
		EnumNames.Emplace(
			FName(*FString::Printf(TEXT("%s::%s"),*EnumName,*AppendEnumItem)),
			MaxEnumValue
		);
	}
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 25
	TargetPlatform->SetEnums(EnumNames,UEnum::ECppForm::EnumClass,EEnumFlags::None,true);
#else
	TargetPlatform->SetEnums(EnumNames,UEnum::ECppForm::EnumClass,true);
#endif
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
}

void FHotPatcherRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherRuntimeModule, HotPatcherRuntime)