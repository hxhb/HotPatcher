// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherEditor.h"
#include "HotPatcherStyle.h"
#include "HotPatcherCommands.h"
#include "SHotPatcher.h"

// ENGINE HEADER
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "DesktopPlatformModule.h"
#include "FlibHotPatcherEditorHelper.h"
#include "HotPatcherLog.h"
#include "LevelEditor.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetTextLibrary.h"
#include "Widgets/Docking/SDockableTab.h"

#if ENGINE_MINOR_VERSION >23
#include "ToolMenus.h"
#include "ToolMenuDelegates.h"
#endif

FExportPatchSettings* GPatchSettings = nullptr;
FExportReleaseSettings* GReleaseSettings = nullptr;

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
#if ENGINE_MINOR_VERSION >23
	AddAssetContentMenu();
#endif
}

void FHotPatcherEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	if(DockTab.IsValid())
	{
		DockTab->RequestCloseTab();
	}
	
	FHotPatcherCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HotPatcherTabName);
	FHotPatcherStyle::Shutdown();
}

void FHotPatcherEditorModule::PluginButtonClicked()
{
	if(!DockTab.IsValid())
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(HotPatcherTabName, FOnSpawnTab::CreateRaw(this, &FHotPatcherEditorModule::OnSpawnPluginTab))
	    .SetDisplayName(LOCTEXT("FHotPatcherTabTitle", "HotPatcher"))
	    .SetMenuType(ETabSpawnerMenuType::Hidden);
	}
	FGlobalTabmanager::Get()->InvokeTab(HotPatcherTabName);
}

// void FHotPatcherEditorModule::PrintUsageMsg()
// {
// 	UWorld* World = GEditor->GetEditorWorldContext().World();
// 	FText DialogText = FText::FromString(
// 		TEXT("This is a Test Message!")
// 	);
// 	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
// }

void FHotPatcherEditorModule::OnTabClosed(TSharedRef<SDockTab> InTab)
{
	DockTab.Reset();
}

TArray<FAssetData> FHotPatcherEditorModule::GetSelectedAssetsInBrowserContent()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> AssetsData;
	ContentBrowserModule.Get().GetSelectedAssets(AssetsData);
	return AssetsData;
}


TSharedRef<class SDockTab> FHotPatcherEditorModule::OnSpawnPluginTab(const class FSpawnTabArgs& InSpawnTabArgs)
{
	return SAssignNew(DockTab,SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("HotPatcherTab", "Hot Patcher"))
		.ToolTipText(LOCTEXT("HotPatcherTabTextToolTip", "Hot Patcher"))
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this,&FHotPatcherEditorModule::OnTabClosed))
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(SHotPatcher)
		];
}

void FHotPatcherEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FHotPatcherCommands::Get().PluginAction);
}

#if ENGINE_MINOR_VERSION >23
void FHotPatcherEditorModule::AddAssetContentMenu()
{
	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		return;
	}
 
	FToolMenuOwnerScoped MenuOwner("CookUtilities");
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	FToolMenuSection& Section = Menu->AddSection("AssetContextCookUtilities", LOCTEXT("CookUtilitiesMenuHeading", "CookUtilities"));;
	
	// Asset Actions sub-menu
	Section.AddSubMenu(
        "CookActionsSubMenu",
        LOCTEXT("CookActionsSubMenuLabel", "Cook Actions"),
        LOCTEXT("CookActionsSubMenuToolTip", "Cook actions"),
        FNewToolMenuDelegate::CreateRaw(this, &FHotPatcherEditorModule::MakeCookActionsSubMenu),
        FUIAction(
            FExecuteAction()
            ),
        EUserInterfaceActionType::Button,
        false, 
        FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions")
        );

	Section.AddDynamicEntry("AddToPatchSettgins", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
       {
           const TAttribute<FText> Label = LOCTEXT("CookUtilities_AddToPatchSettgins", "Add To Patch Settgins");
           const TAttribute<FText> ToolTip = LOCTEXT("CookUtilities_AddToPatchSettginsTooltip", "Add Selected Assets To HotPatcher Patch Settgins");
           const FSlateIcon Icon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Duplicate");
           const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateRaw(this,&FHotPatcherEditorModule::OnAddToPatchSettings);
	
           InSection.AddMenuEntry("CookUtilities_AddToPatchSettgins", Label, ToolTip, Icon, UIAction);
       }));
}

void FHotPatcherEditorModule::OnAddToPatchSettings(const FToolMenuContext& MenuContent)
{
	if(!DockTab.IsValid() || !GPatchSettings)
	{
		PluginButtonClicked();
	}
	TArray<FAssetData> AssetsData = GetSelectedAssetsInBrowserContent();
	TArray<FPatcherSpecifyAsset> AssetsSoftPath;

	for(const auto& AssetData:AssetsData)
	{
		FPatcherSpecifyAsset PatchSettingAssetElement;
		FSoftObjectPath AssetObjectPath;
		AssetObjectPath.SetPath(AssetData.ObjectPath.ToString());
		PatchSettingAssetElement.Asset = AssetObjectPath;
		AssetsSoftPath.AddUnique(PatchSettingAssetElement);
	}
	GPatchSettings->IncludeSpecifyAssets.Append(AssetsSoftPath);
}

void FHotPatcherEditorModule::MakeCookActionsSubMenu(UToolMenu* Menu)
{
	
	FToolMenuSection& Section = Menu->AddSection("CookActionsSection");

	for (auto Platform : GetAllCookPlatforms())
	{
		Section.AddMenuEntry(
            NAME_None,
            FText::Format(LOCTEXT("Platform", "{0}"), UKismetTextLibrary::Conv_StringToText(UFlibPatchParserHelper::GetEnumNameByValue(Platform))),
            FText(),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateRaw(this, &FHotPatcherEditorModule::OnCookPlatform, Platform)
            )
        );
	}
}
#endif

TArray<ETargetPlatform> FHotPatcherEditorModule::GetAllCookPlatforms() const
{
	TArray<ETargetPlatform> TargetPlatforms;//{ETargetPlatform::Android_ASTC,ETargetPlatform::IOS,ETargetPlatform::WindowsNoEditor};
#if ENGINE_MINOR_VERSION > 21
	UEnum* FoundEnum = StaticEnum<ETargetPlatform>();
#else
	FString EnumTypeName = ANSI_TO_TCHAR(UFlibPatchParserHelper::GetCPPTypeName<ETargetPlatform>().c_str());
	UEnum* FoundEnum = FindObject<UEnum>(ANY_PACKAGE, *EnumTypeName, true); 
#endif
	if (FoundEnum)
	{
		for(int32 index =1;index < FoundEnum->GetMaxEnumValue();++index)
		{
			TargetPlatforms.AddUnique((ETargetPlatform)(index));
		}
			
	}
	return TargetPlatforms;
}

void FHotPatcherEditorModule::OnCookPlatform(ETargetPlatform Platform)
{
	TArray<FAssetData> AssetsData = GetSelectedAssetsInBrowserContent();
	TArray<UPackage*> AssetsPackage;

	for(const auto& AssetData:AssetsData)
	{
		AssetsPackage.AddUnique(AssetData.GetPackage());
		UE_LOG(LogHotPatcher,Log,TEXT("Cook Package %s Platform %s"),*AssetData.PackagePath.ToString(),*UFlibPatchParserHelper::GetEnumNameByValue(Platform));
	}
	FString CookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
	FString CookForPlatform = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
	TArray<FString> CookForPlatforms{CookForPlatform};
	
	for(const auto& AssetPacakge:AssetsPackage)
	{
		FString CookedSavePath = UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(CookedDir,AssetPacakge, CookForPlatform);
		
    	bool bCookStatus = UFlibHotPatcherEditorHelper::CookPackage(AssetPacakge,CookForPlatforms,CookedDir);
		auto Msg = FText::Format(
            LOCTEXT("CookAssetsNotify", "Cook Platform {1} for {0} {2}!"),
            UKismetTextLibrary::Conv_StringToText(AssetPacakge->FileName.ToString()),
            UKismetTextLibrary::Conv_StringToText(UFlibPatchParserHelper::GetEnumNameByValue(Platform)),
            bCookStatus ? UKismetTextLibrary::Conv_StringToText(TEXT("Successfuly")):UKismetTextLibrary::Conv_StringToText(TEXT("Faild"))
            );
		SNotificationItem::ECompletionState CookStatus = bCookStatus ? SNotificationItem::CS_Success:SNotificationItem::CS_Fail;
		UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg,CookedSavePath,CookStatus);
	}
	
}

void FHotPatcherEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FHotPatcherCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherEditorModule, HotPatcherEditor)