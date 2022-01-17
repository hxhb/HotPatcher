// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherCore.h"
#include "HotPatcherSettings.h"

// ENGINE HEADER

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "DesktopPlatformModule.h"
#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherLog.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetTextLibrary.h"
#include "PakFileUtilities.h"
#include "Cooker/MultiCooker/FlibMultiCookerHelper.h"
#include "Cooker/MultiCooker/FMultiCookerSettings.h"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "CreatePatch/PatcherProxy.h"


FExportPatchSettings* GPatchSettings = nullptr;
FExportReleaseSettings* GReleaseSettings = nullptr;
bool GCookLog = (bool)ENABLE_COOK_LOG;

static const FName HotPatcherTabName("HotPatcher");

#define LOCTEXT_NAMESPACE "FHotPatcherCoreModule"


FHotPatcherCoreModule& FHotPatcherCoreModule::Get()
{
	FHotPatcherCoreModule& Module = FModuleManager::GetModuleChecked<FHotPatcherCoreModule>("HotPatcherCore");
	return Module;
}

void FHotPatcherCoreModule::StartupModule()
{

}



void FHotPatcherCoreModule::ShutdownModule()
{

}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherCoreModule, HotPatcherCore)