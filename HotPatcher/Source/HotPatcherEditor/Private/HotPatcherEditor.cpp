// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherEditor.h"
#include "HotPatcherStyle.h"
#include "HotPatcherCommands.h"
#include "SHotPatcher.h"
#include "HotPatcherSettings.h"

// ENGINE HEADER
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "DesktopPlatformModule.h"
#include "FlibHotPatcherEditorHelper.h"
#include "HotPatcherLog.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetTextLibrary.h"
#include "PakFileUtilities.h"

#if ENGINE_MAJOR_VERSION < 5
#include "Widgets/Docking/SDockableTab.h"
#endif

#if ENGINE_MAJOR_VERSION >4 || ENGINE_MINOR_VERSION >23
#include "ToolMenus.h"
#include "ToolMenuDelegates.h"
#endif

FExportPatchSettings* GPatchSettings = nullptr;
FExportReleaseSettings* GReleaseSettings = nullptr;

static const FName HotPatcherTabName("HotPatcher");

#define LOCTEXT_NAMESPACE "FHotPatcherEditorModule"


void MakeProjectSettingsForHotPatcher()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Hot Patcher",
            LOCTEXT("HotPatcherSettingsName", "Hot Patcher"),
            LOCTEXT("HotPatcherSettingsDescroption", "Configure the HotPatcher plugin"),
            GetMutableDefault<UHotPatcherSettings>());
	}
}

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
	MakeProjectSettingsForHotPatcher();
#if ENGINE_MAJOR_VERSION > 4 ||  ENGINE_MINOR_VERSION >23
	AddAssetContentMenu();
	AddFolderContentMenu();
#endif
	FCoreUObjectDelegates::OnObjectSaved.AddRaw(this,&FHotPatcherEditorModule::OnObjectSaved);
	
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

TArray<FString> FHotPatcherEditorModule::GetSelectedFolderInBrowserContent()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FString> Folders;
	ContentBrowserModule.Get().GetSelectedFolders(Folders);
	return Folders;
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

#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >23
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
	Section.AddSubMenu(
        "CookAndPakActionsSubMenu",
        LOCTEXT("CookAndPakActionsSubMenuLabel", "Cook And Pak Actions"),
        LOCTEXT("CookAndPakActionsSubMenuToolTip", "Cook And Pak actions"),
        FNewToolMenuDelegate::CreateRaw(this, &FHotPatcherEditorModule::MakeCookAndPakActionsSubMenu),
        FUIAction(
            FExecuteAction()
            ),
        EUserInterfaceActionType::Button,
        false, 
        FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.SaveAllCurrentFolder")
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
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	for (auto Platform : GetAllCookPlatforms())
	{
		if(Settings->bWhiteListCookInEditor && !Settings->PlatformWhitelists.Contains(Platform))
			continue;
		Section.AddMenuEntry(
            FName(UFlibPatchParserHelper::GetEnumNameByValue(Platform)),
            FText::Format(LOCTEXT("Platform", "{0}"), UKismetTextLibrary::Conv_StringToText(UFlibPatchParserHelper::GetEnumNameByValue(Platform))),
            FText(),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateRaw(this, &FHotPatcherEditorModule::OnCookPlatform, Platform)
            )
        );
	}
}

void FHotPatcherEditorModule::MakeCookAndPakActionsSubMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection("CookAndPakActionsSection");
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	for (auto Platform : GetAllCookPlatforms())
	{
		if(Settings->bWhiteListCookInEditor && !Settings->PlatformWhitelists.Contains(Platform))
			continue;
		Section.AddMenuEntry(
            FName(UFlibPatchParserHelper::GetEnumNameByValue(Platform)),
            FText::Format(LOCTEXT("Platform", "{0}"), UKismetTextLibrary::Conv_StringToText(UFlibPatchParserHelper::GetEnumNameByValue(Platform))),
            FText(),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateRaw(this, &FHotPatcherEditorModule::OnCookAndPakPlatform, Platform)
            )
        );
	}
}

void FHotPatcherEditorModule::AddFolderContentMenu()
{
	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		return;
	}
 
	FToolMenuOwnerScoped MenuOwner("PatchUtilities");
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.FolderContextMenu");
	FToolMenuSection& Section = Menu->AddSection("FolderContextCookUtilities", LOCTEXT("PatchUtilitiesMenuHeading", "PatchUtilities"));;
	
	Section.AddDynamicEntry("AddToPatchSettgins", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
       {
           const TAttribute<FText> Label = LOCTEXT("PatchUtilities_AddFolderToPatchSettgins", "Add Folder To Patch Settgins");
           const TAttribute<FText> ToolTip = LOCTEXT("PatchUtilities_AddFolderToPatchSettginsTooltip", "Add Selected Folder To HotPatcher Patch Settgins");
           const FSlateIcon Icon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Duplicate");
           const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateRaw(this,&FHotPatcherEditorModule::OnFolderAddToPatchSettings);
	
           InSection.AddMenuEntry("CookUtilities_AddFolderToPatchSettgins", Label, ToolTip, Icon, UIAction);
       }));
}

void FHotPatcherEditorModule::OnFolderAddToPatchSettings(const FToolMenuContext& MenuContent)
{
	if(!DockTab.IsValid() || !GPatchSettings)
	{
		PluginButtonClicked();
	}
	TArray<FString> Folders = GetSelectedFolderInBrowserContent();

	for(const auto& folder:Folders)
	{
		FDirectoryPath Path;
		Path.Path = folder;
		GPatchSettings->AssetIncludeFilters.Add(Path);
	}
}
#endif

TArray<ETargetPlatform> FHotPatcherEditorModule::GetAllCookPlatforms() const
{
	TArray<ETargetPlatform> TargetPlatforms;//{ETargetPlatform::Android_ASTC,ETargetPlatform::IOS,ETargetPlatform::WindowsNoEditor};
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 21
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

	FString CookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
	FString CookForPlatform = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
	TArray<FString> CookForPlatforms{CookForPlatform};
	
	for(int32 index=0;index < AssetsData.Num();++index)
	{
		FString CookedSavePath = UFlibHotPatcherEditorHelper::GetCookAssetsSaveDir(CookedDir,AssetsData[index].PackageName.ToString(), CookForPlatform);
		UE_LOG(LogHotPatcher,Log,TEXT("Cook Package %s Platform %s"),*AssetsData[index].PackagePath.ToString(),*UFlibPatchParserHelper::GetEnumNameByValue(Platform));
		
    	bool bCookStatus = UFlibHotPatcherEditorHelper::CookPackage(AssetsData[index],AssetsData[index].GetPackage(),CookForPlatforms,CookedDir);
		auto Msg = FText::Format(
            LOCTEXT("CookAssetsNotify", "Cook Platform {1} for {0} {2}!"),
            UKismetTextLibrary::Conv_StringToText(AssetsData[index].PackageName.ToString()),
            UKismetTextLibrary::Conv_StringToText(UFlibPatchParserHelper::GetEnumNameByValue(Platform)),
            bCookStatus ? UKismetTextLibrary::Conv_StringToText(TEXT("Successfuly")):UKismetTextLibrary::Conv_StringToText(TEXT("Faild"))
            );
		SNotificationItem::ECompletionState CookStatus = bCookStatus ? SNotificationItem::CS_Success:SNotificationItem::CS_Fail;
		UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg,CookedSavePath,CookStatus);
	}
	
}

void FHotPatcherEditorModule::OnCookAndPakPlatform(ETargetPlatform Platform)
{
	TArray<FAssetData> AssetsData = GetSelectedAssetsInBrowserContent();
	TArray<UPackage*> AssetsPackage;
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	OnCookPlatform(Platform);
	FString CookForPlatform = UFlibPatchParserHelper::GetEnumNameByValue(Platform);
	FString AbsProjectPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	TArray<FString> FinalCookCommands;
	for(const auto& AssetData:AssetsData)
	{
		TArray<FString> CookCommands;
		UFLibAssetManageHelperEx::MakePakCommandFromLongPackageName(AbsProjectPath, CookForPlatform, AssetData.PackageName.ToString(), Settings->CookParams, CookCommands,[](const TArray<FString>&, const FString&, const FString&){});
		FinalCookCommands.Append(CookCommands);
	}
	if(!!FinalCookCommands.Num())
	{
		FString TempPakCommandsSavePath =  FPaths::Combine(AbsProjectPath,Settings->TempPakDir,FDateTime::UtcNow().ToString()+TEXT("_")+CookForPlatform+TEXT(".txt"));
		FString TempPakSavePath =  FPaths::Combine(AbsProjectPath,Settings->TempPakDir,FDateTime::UtcNow().ToString()+TEXT("_")+CookForPlatform+TEXT(".pak"));
		FFileHelper::SaveStringArrayToFile(FinalCookCommands,*TempPakCommandsSavePath);
		if(FPaths::FileExists(TempPakCommandsSavePath))
		{
			FString UnrealPakOptionsSinglePak(
                            FString::Printf(
                                TEXT("%s -create=%s"),
                                *(TEXT("\"") + TempPakSavePath + TEXT("\"")),
                                *(TEXT("\"") + TempPakCommandsSavePath + TEXT("\""))
                            )
                        );
			ExecuteUnrealPak(*UnrealPakOptionsSinglePak);
			if (FPaths::FileExists(TempPakSavePath))
			{
				FText Msg = LOCTEXT("SavedPakFileMsg", "Successd to Package the patch as Pak.");
				UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, TempPakSavePath);
			}	
		}
	}
}

void FHotPatcherEditorModule::OnObjectSaved(UObject* ObjectSaved)
{
	if (GIsCookerLoadingPackage)
	{
		// This is the cooker saving a cooked package, ignore
		return;
	}

	UPackage* Package = ObjectSaved->GetOutermost();
	if (Package == nullptr || Package == GetTransientPackage())
	{
		return;
	}


	// Register the package filename as modified. We don't use the cache because the file may not exist on disk yet at this point
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), Package->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().ScanFilesSynchronous(TArray<FString>{PackageFilename},false);
}

void FHotPatcherEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FHotPatcherCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherEditorModule, HotPatcherEditor)