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
#include "Chunker/HotPatcherPrimaryLabel.h"
#include "Chunker/HPLProxy.h"
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
	UE_LOG(LogHotPatcher,Log,TEXT("GCookLog is %s!!!"),GCookLog ? TEXT("TRUE"): TEXT("FALSE"));
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 24)
	FCoreDelegates::OnEnginePreExit.AddRaw(this,&FHotPatcherCoreModule::OnPreEngineExit_Commandlet);
#endif
}

void FHotPatcherCoreModule::ShutdownModule()
{
	
}

void FHotPatcherCoreModule::OnPreEngineExit_Commandlet()
{
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();

	SetHPLStagedBuildsDirConfig();
	if(CommandletHelper::IsCookCommandlet() && Settings->bUseHPL)
	{
		UHPLProxy* UPLProxy = NewObject<UHPLProxy>();
		UPLProxy->AddToRoot();
		UPLProxy->Init(nullptr);
		UPLProxy->DoExport();
		UPLProxy->RemoveFromRoot();
	}
}


void FHotPatcherCoreModule::SetHPLStagedBuildsDirConfig()
{
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	if(CommandletHelper::IsCookCommandlet())
	{
		UProjectPackagingSettings* PackageSettings = GetMutableDefault<UProjectPackagingSettings>();
		TArray<ETargetPlatform> TargetPlatforms = CommandletHelper::GetCookCommandletTargetPlatforms();
		
		FString StagedBuildsDirRelativeToContent = UFlibPatchParserHelper::ReplaceMarkPath(Settings->TempStagedBuildsDir);

		// for(const auto& CookTargetPlatformName:CommandletHelper::GetCookCommandletTargetPlatformName())
		{
			// if(!CookTargetPlatformName.IsEmpty())
			{
				// ITargetPlatform* TargetPlatform = UFlibHotPatcherCoreHelper::GetPlatformByName(CookTargetPlatformName);
				// FString CookIniPlatformName = TargetPlatform->IniPlatformName();
				
				FPaths::MakePathRelativeTo(StagedBuildsDirRelativeToContent,*FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
				// StagedBuildsDirRelativeToContent = FPaths::Combine(StagedBuildsDirRelativeToContent,CookIniPlatformName);
				
				bool bContainStageBuild = false;
				int32 FoundIndex = INDEX_NONE;
				if(!StagedBuildsDirRelativeToContent.IsEmpty())
				{
					TArray<FDirectoryPath>& DirectoriesToAlwaysStageAsNonUFS = PackageSettings->DirectoriesToAlwaysStageAsNonUFS;
					for(int32 index = 0 ;index < DirectoriesToAlwaysStageAsNonUFS.Num();++index)
					{
						if(DirectoriesToAlwaysStageAsNonUFS[index].Path.Equals(StagedBuildsDirRelativeToContent))
						{
							FoundIndex = index;
							bContainStageBuild = true;
							break;
						}
					}
			
					if(!bContainStageBuild && Settings->bUseHPL)
					{
						FDirectoryPath AddPath;
						AddPath.Path = StagedBuildsDirRelativeToContent;
						DirectoriesToAlwaysStageAsNonUFS.Add(AddPath);
					}
					if(!Settings->bUseHPL && bContainStageBuild)
					{
						DirectoriesToAlwaysStageAsNonUFS.RemoveAt(FoundIndex);
					}
#if ENGINE_MAJOR_VERSION >= 5
					PackageSettings->TryUpdateDefaultConfigFile();
#else
					PackageSettings->UpdateDefaultConfigFile();
#endif
				}
			}
		}
	}
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherCoreModule, HotPatcherCore)