// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#pragma once

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >23
	#include "ToolMenuContext.h"
	#include "ToolMenu.h"
#endif

#include "CoreMinimal.h"
#include "ETargetPlatform.h"
#include "FlibHotPatcherEditorHelper.h"
#include "Modules/ModuleManager.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/FExportReleaseSettings.h"

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION>=26
	#define InvokeTab TryInvokeTab
#endif

HOTPATCHEREDITOR_API void ReceiveOutputMsg(const FString& InMsg);

class FToolBarBuilder;
class FMenuBuilder;
extern FExportPatchSettings* GPatchSettings;
extern FExportReleaseSettings* GReleaseSettings;

class FHotPatcherEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();
	
public:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >23
	void AddAssetContentMenu();
	void OnAddToPatchSettings(const FToolMenuContext& MenuContent);
	void MakeCookActionsSubMenu(UToolMenu* Menu);
	void MakeCookAndPakActionsSubMenu(UToolMenu* Menu);
	void AddFolderContentMenu();
	void OnFolderAddToPatchSettings(const FToolMenuContext& MenuContent);
#endif
	TArray<ETargetPlatform> GetAllCookPlatforms()const;
	void OnCookPlatform(ETargetPlatform Platform);
	void OnCookAndPakPlatform(ETargetPlatform Platform);

	void OnObjectSaved( UObject* ObjectSaved );

private:
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& InSpawnTabArgs);
	void OnTabClosed(TSharedRef<SDockTab> InTab);
	TArray<FAssetData> GetSelectedAssetsInBrowserContent();
	TArray<FString> GetSelectedFolderInBrowserContent();
private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TSharedPtr<SDockTab> DockTab;
};
