// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "HotPatcherRuntime.h"
#include "FlibPatchParserHelper.h"
#include "ETargetPlatform.h"
#include "Misc/EnumRange.h"
#include "HotPatcherLog.h"

#define LOCTEXT_NAMESPACE "FHotPatcherRuntimeModule"

bool GForceSingleThread = (bool)FORCE_SINGLE_THREAD;

void FHotPatcherRuntimeModule::StartupModule()
{
	UE_LOG(LogHotPatcher,Display,TEXT("HotPatcherRuntime StartupModule"));
#if AUTOLOAD_SHADERLIB_AT_RUNTIME && !WITH_EDITOR
	UFlibPakHelper::LoadHotPatcherAllShaderLibrarys();
#endif
}

void FHotPatcherRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherRuntimeModule, HotPatcherRuntime)