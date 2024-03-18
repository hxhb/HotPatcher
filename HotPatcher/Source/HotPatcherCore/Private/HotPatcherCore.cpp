// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherCore.h"
#include "HotPatcherSettings.h"

// ENGINE HEADER

#include "AssetToolsModule.h"
#include "CommandletHelper.h"
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

#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "CreatePatch/PatcherProxy.h"
#include "Settings/ProjectPackagingSettings.h"
#include "ThreadUtils/FProcWorkerThread.hpp"


FExportPatchSettings* GPatchSettings = nullptr;
FExportReleaseSettings* GReleaseSettings = nullptr;
bool GCookLog = (bool)ENABLE_COOK_LOG;
FString GToolName = TOOL_NAME;
int32 GToolMainVersion = CURRENT_VERSION_ID;
int32 GToolPatchVersion = CURRENT_PATCH_ID;
FString GRemoteVersionFile = REMOTE_VERSION_FILE;

static const FName HotPatcherTabName("HotPatcher");

#define LOCTEXT_NAMESPACE "FHotPatcherCoreModule"

void ReceiveOutputMsg(FProcWorkerThread* Worker,const FString& InMsg)
{
	FString FindItem(TEXT("Display:"));
	int32 Index= InMsg.Len() - InMsg.Find(FindItem)- FindItem.Len();
	if (InMsg.Contains(TEXT("Error:")))
	{
		UE_LOG(LogHotPatcher, Error, TEXT("%s"), *InMsg);
	}
	else if (InMsg.Contains(TEXT("Warning:")))
	{
		UE_LOG(LogHotPatcher, Warning, TEXT("%s"), *InMsg);
	}
	else
	{
		UE_LOG(LogHotPatcher, Display, TEXT("%s"), *InMsg.Right(Index));
	}
}

FHotPatcherCoreModule& FHotPatcherCoreModule::Get()
{
	FHotPatcherCoreModule& Module = FModuleManager::GetModuleChecked<FHotPatcherCoreModule>("HotPatcherCore");
	return Module;
}


void FHotPatcherCoreModule::StartupModule()
{
	FParse::Bool(FCommandLine::Get(),TEXT("-cooklog"),GCookLog);
	UE_LOG(LogHotPatcher,Display,TEXT("GCookLog is %s!!!"),GCookLog ? TEXT("TRUE"): TEXT("FALSE"));
}

void FHotPatcherCoreModule::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherCoreModule, HotPatcherCore)