// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherEditor.h"
#include "HotPatcherStyle.h"
#include "HotPatcherCommands.h"
#include "SHotPatcher.h"

#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "DesktopPlatformModule.h"
#include "LevelEditor.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"


static const FName HotPatcherTabName("HotPatcher");

#define LOCTEXT_NAMESPACE "FHotPatcherEditorModule"

void FHotPatcherEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FHotPatcherStyle::Initialize();
	FHotPatcherStyle::ReloadTextures();

	FHotPatcherCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FHotPatcherCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FHotPatcherEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FHotPatcherEditorModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	 {
	 	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	 	ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FHotPatcherEditorModule::AddToolbarExtension));

	 	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	 }
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(HotPatcherTabName, FOnSpawnTab::CreateRaw(this, &FHotPatcherEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FHotPatcherTabTitle", "HotPatcher"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

}

void FHotPatcherEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FHotPatcherStyle::Shutdown();

	FHotPatcherCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HotPatcherTabName);
}

void FHotPatcherEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(HotPatcherTabName);
	// PrintUsageMsg();
}

void FHotPatcherEditorModule::PrintUsageMsg()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	FText DialogText = FText::FromString(
		TEXT("This is a Test Message!")
	);
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}


TSharedRef<class SDockTab> FHotPatcherEditorModule::OnSpawnPluginTab(const class FSpawnTabArgs& InSpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("HotPatcherTab", "Hot Patcher"))
		.ToolTipText(LOCTEXT("WidgetGalleryTabTextToolTip", "Switch to the widget gallery."))
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			// MakeWidgetGallery()
			SNew(SHotPatcher)
		];
}

void FHotPatcherEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FHotPatcherCommands::Get().PluginAction);
}



void FHotPatcherEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FHotPatcherCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherEditorModule, HotPatcherEditor)