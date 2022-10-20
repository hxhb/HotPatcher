// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR_SECTION
	#include "ToolMenuContext.h"
	#include "ToolMenu.h"
#endif

#include "HotPatcherSettings.h"
#include "ETargetPlatform.h"
#include "FlibHotPatcherCoreHelper.h"
#include "HotPatcherSettings.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/FExportReleaseSettings.h"
#include "Model/FHotPatcherContextBase.h"
#include "HotPatcherActionManager.h"

// engine header
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ContentBrowserDelegates.h"
#include "MissionNotificationProxy.h"
#include "ThreadUtils/FProcWorkerThread.hpp"

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION>=26
	#define InvokeTab TryInvokeTab
#endif

class FToolBarBuilder;
class FMenuBuilder;
extern FExportPatchSettings* GPatchSettings;
extern FExportReleaseSettings* GReleaseSettings;

struct FContentBrowserSelectedInfo
{
	TArray<FAssetData> SelectedAssets;
	TArray<FString> SelectedPaths;
	TArray<FName> OutAllAssetPackages;
};

class HOTPATCHEREDITOR_API FHotPatcherEditorModule : public IModuleInterface
{
public:
	static FHotPatcherEditorModule& Get();
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void OpenDockTab();
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
public:
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedPtr<FProcWorkerThread> CreateProcMissionThread(const FString& Bin, const FString& Command, const FString& MissionName);
	
	TSharedPtr<FProcWorkerThread> RunProcMission(const FString& Bin, const FString& Command, const FString& MissionName);

	
#if WITH_EDITOR_SECTION
	void CreateRootMenu();
	void CreateAssetContextMenu(FToolMenuSection& InSection);
	void ExtendContentBrowserAssetSelectionMenu();
	void ExtendContentBrowserPathSelectionMenu();
	void MakeCookActionsSubMenu(UToolMenu* Menu);
	void MakeCookAndPakActionsSubMenu(UToolMenu* Menu);
	void MakePakExternalActionsSubMenu(UToolMenu* Menu);
	void MakeHotPatcherPresetsActionsSubMenu(UToolMenu* Menu);
	void OnAddToPatchSettings(const FToolMenuContext& MenuContent);
#endif
	TArray<ETargetPlatform> GetAllCookPlatforms()const;
	void OnCookPlatform(ETargetPlatform Platform);
	void OnCookAndPakPlatform(ETargetPlatform Platform, bool bAnalysicDependencies);
	void CookAndPakByAssetsAndFilters(TArray<FPatcherSpecifyAsset> IncludeAssets,TArray<FDirectoryPath> IncludePaths,TArray<ETargetPlatform> Platforms,bool bForceStandalone = false);
	void OnPakExternal(FPakExternalInfo Config);
	void OnPakPreset(FExportPatchSettings Config);
	void OnObjectSaved( UObject* ObjectSaved );


	FExportPatchSettings MakeTempPatchSettings(
		const FString& Name,
		const TArray<FDirectoryPath>& AssetIncludeFilters,
		const TArray<FPatcherSpecifyAsset>& IncludeSpecifyAssets,
		const TArray<FPlatformExternAssets>& ExternFiles,
		const TArray<ETargetPlatform>& PakTargetPlatforms,
		bool bCook
	);
private:
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& InSpawnTabArgs);
	void OnTabClosed(TSharedRef<SDockTab> InTab);
	TArray<FAssetData> GetSelectedAssetsInBrowserContent();
	TArray<FString> GetSelectedFolderInBrowserContent();
	
private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<SDockTab> DockTab;
	mutable TSharedPtr<FProcWorkerThread> mProcWorkingThread;
	UMissionNotificationProxy* MissionNotifyProay;
	TSharedPtr<FExportPatchSettings> PatchSettings;
	TArray<class UPatcherProxy*> Proxys;
};
