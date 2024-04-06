// Copyright 2019 Lipeng Zha, Inc. All Rights Reserved.

#include "HotPatcherEditor.h"
#include "HotPatcherStyle.h"
#include "HotPatcherCommands.h"
#include "SHotPatcher.h"
#include "HotPatcherSettings.h"
#include "Templates/HotPatcherTemplateHelper.hpp"
#include "Cooker/MultiCooker/SingleCookerProxy.h"
#include "CreatePatch/PatcherProxy.h"
#include "Cooker/MultiCooker/FlibHotCookerHelper.h"
#include "HotPatcherActionManager.h"
#include "FlibHotPatcherCoreHelper.h"
#include "FlibHotPatcherEditorHelper.h"
#include "HotPatcherLog.h"

// ENGINE HEADER
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "DesktopPlatformModule.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetTextLibrary.h"
#include "PakFileUtilities.h"
#include "Misc/EngineVersionComparison.h"

#if !UE_VERSION_OLDER_THAN(5,1,0)
	typedef FAppStyle FEditorStyle;
#endif

#if ENGINE_MAJOR_VERSION < 5
#include "Widgets/Docking/SDockableTab.h"
#endif

#if WITH_EDITOR_SECTION
#include "ToolMenus.h"
#include "ToolMenuDelegates.h"
#include "ContentBrowserMenuContexts.h"
#include "CreatePatch/AssetActions/AssetTypeActions_PrimaryAssetLabel.h"
#endif

static const FName HotPatcherTabName("HotPatcher");

DEFINE_LOG_CATEGORY(LogHotPatcherEdotor)

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

FHotPatcherEditorModule& FHotPatcherEditorModule::Get()
{
	FHotPatcherEditorModule& Module = FModuleManager::GetModuleChecked<FHotPatcherEditorModule>("HotPatcherEditor");
	return Module;
}

void FHotPatcherEditorModule::StartupModule()
{
	UE_LOG(LogHotPatcherEdotor,Display,TEXT("FHotPatcherEditorModule StartupModule"));
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FHotPatcherStyle::Initialize();
	FHotPatcherStyle::ReloadTextures();
	FHotPatcherCommands::Register();

	FHotPatcherActionManager::Get().Init();

	
#if !UE_VERSION_OLDER_THAN(5,1,0)
	StyleSetName = FEditorStyle::GetAppStyleSetName().ToString();
#else
	StyleSetName = FEditorStyle::GetStyleSetName().ToString();
#endif

	
	FHotPatcherDelegates::Get().GetNotifyFileGenerated().AddLambda([](FText Msg,const FString& File)
	{
		UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg,File);
	});
	
	if(::IsRunningCommandlet())
		return;
	
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FCoreUObjectDelegates::OnObjectSaved.AddRaw(this,&FHotPatcherEditorModule::OnObjectSaved);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	MakeProjectSettingsForHotPatcher();

	MissionNotifyProay = NewObject<UMissionNotificationProxy>();
	MissionNotifyProay->AddToRoot();
	
	PluginCommands = MakeShareable(new FUICommandList);
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FHotPatcherEditorModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

#ifndef DISABLE_PLUGIN_TOOLBAR_MENU
		// settings
	 	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
#if ENGINE_MAJOR_VERSION > 4
		FName ExtensionHook = "Play";
#else
		FName ExtensionHook = "Settings";
#endif
	 	ToolbarExtender->AddToolBarExtension(ExtensionHook, EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FHotPatcherEditorModule::AddToolbarExtension));
		
	 	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
#endif
	}

	TSharedRef<FUICommandList> CommandList = LevelEditorModule.GetGlobalLevelEditorActions();
	CommandList->UnmapAction(FHotPatcherCommands::Get().CookSelectedAction);
	CommandList->UnmapAction(FHotPatcherCommands::Get().CookAndPakSelectedAction);
	CommandList->UnmapAction(FHotPatcherCommands::Get().AddToPakSettingsAction);

#if WITH_EDITOR_SECTION
	ExtendContentBrowserAssetSelectionMenu();
	ExtendContentBrowserPathSelectionMenu();
	
	CreateRootMenu();

	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_PrimaryAssetLabel));

#endif
}



void FHotPatcherEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FHotPatcherActionManager::Get().Shutdown();
	FHotPatcherCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HotPatcherTabName);
	FHotPatcherStyle::Shutdown();
}

void FHotPatcherEditorModule::OpenDockTab()
{
	if(!DockTab.IsValid() || !GPatchSettings)
	{
		FSHotPatcherContext Context;
		Context.Category = TEXT("Patcher");
		Context.ActionName = TEXT("ByPatch");
		PluginButtonClicked(Context);
	}
}

void FHotPatcherEditorModule::PluginButtonClicked(const FSHotPatcherContext& Context)
{
	if(DockTab.IsValid())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HotPatcherTabName);
		DockTab.Reset();
	}
	
	if (!DockTab.IsValid())
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(HotPatcherTabName, FOnSpawnTab::CreateLambda([=](const class FSpawnTabArgs& InSpawnTabArgs)
		{
			return SpawnHotPatcherTab(Context);
		}))
		.SetDisplayName(LOCTEXT("FHotPatcherTabTitle", "HotPatcher"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	}
	FGlobalTabmanager::Get()->InvokeTab(HotPatcherTabName);
}

void FHotPatcherEditorModule::OnTabClosed(TSharedRef<SDockTab> InTab)
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HotPatcherTabName);
	DockTab.Reset();
}

void FHotPatcherEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	FSlateIcon EmptyIcon(FHotPatcherStyle::GetStyleSetName(), "HotPatcher.PluginAction");
	Builder.AddSubMenu(
		FText::FromString(TEXT("HotPatcher")),
		FText::FromString(TEXT("HotPatcher")),
		FNewMenuDelegate::CreateLambda([=](FMenuBuilder& Menu)
		{
			Menu.AddWidget(this->HandlePickingModeContextMenu(),FText::FromString(TEXT("")),true);
		}),
		false,
		EmptyIcon);
}

void FHotPatcherEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	// Builder.AddToolBarButton(FHotPatcherCommands::Get().PluginAction);
	
	FSlateIcon EmptyIcon(FHotPatcherStyle::GetStyleSetName(), "HotPatcher.PluginAction");
	Builder.AddComboButton(
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateLambda([]()->bool{return true;}),
				FGetActionCheckState()
			),
			FOnGetContent::CreateLambda([this]()->TSharedRef<SWidget>
			{
				return this->HandlePickingModeContextMenu();
			}),
			FText::FromString(TEXT("HotPatcher")),
			FText::GetEmpty(),
			EmptyIcon
		);
}

TSharedRef<SWidget> FHotPatcherEditorModule::HandlePickingModeContextMenu()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr);
	FHotPatcherActionManager::FHotPatcherActionsType Actions = FHotPatcherActionManager::Get().GetHotPatcherActions();
	TArray<FString> CategoryNames;
	Actions.GetKeys(CategoryNames);

	// for Main Page
	{
		FSlateIcon HotPatcherIcon(FHotPatcherStyle::GetStyleSetName(), "HotPatcher.PluginAction");
		FSHotPatcherContext Context;
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("MAIN")),
			FText::FromString(TEXT("MAIN")),
			HotPatcherIcon,
			FUIAction(FExecuteAction::CreateLambda([=]()
			{
				this->PluginButtonClicked(Context);
			})));
		MenuBuilder.AddMenuSeparator();
	}
	
	for(int32 index = 0; index < CategoryNames.Num(); ++index)
	{
		TArray<FString> ActionKeys;
		Actions.Find(CategoryNames[index])->GetKeys(ActionKeys);

		FString CategoryName = CategoryNames[index];
		if(CategoryName.Equals(TEXT("NoSlate"))){ continue; }
		if(index && !(index % 2))
		{
			MenuBuilder.AddMenuSeparator();
		}
		MenuBuilder.AddSubMenu(
			FText::FromString(CategoryName),
			FText::FromString(CategoryName),
			FNewMenuDelegate::CreateLambda([this,CategoryName,ActionKeys](FMenuBuilder& Menu)
			{
				for(const auto& ActionName:ActionKeys)
				{
					FSHotPatcherContext Context;
					Context.Category = CategoryName;
					Context.ActionName = ActionName;
					Menu.AddMenuEntry(
						FText::FromString(ActionName),
						FText::FromString(ActionName),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([=]()
						{
							this->PluginButtonClicked(Context);
						}))
					);
				}
			}),
			false
		);
	}
	return MenuBuilder.MakeWidget();
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


TSharedRef<SDockTab> FHotPatcherEditorModule::SpawnHotPatcherTab(const FSHotPatcherContext& Context)
{
	return SAssignNew(DockTab,SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("HotPatcherTab", "Hot Patcher"))
		.ToolTipText(LOCTEXT("HotPatcherTabTextToolTip", "Hot Patcher"))
		.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this,&FHotPatcherEditorModule::OnTabClosed))
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(SHotPatcher,Context)
		];
}

#if WITH_EDITOR_SECTION 

void FHotPatcherEditorModule::CreateRootMenu()
{
	UToolMenu* RootMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AddNewContextMenu");
	FToolMenuSection& RootSection = RootMenu->AddSection("HotPatcherUtilities", LOCTEXT("HotPatcherUtilities", "HotPatcherUtilities"));;
	RootSection.AddSubMenu(
		"PatcherPresetsActionsSubMenu",
		LOCTEXT("PatcherPresetsActionsSubMenuLabel", "Preset Actions"),
		LOCTEXT("PatcherPresetsActionsSubMenuToolTip", "Preset Actions"),
		FNewToolMenuDelegate::CreateRaw(this, &FHotPatcherEditorModule::MakeHotPatcherPresetsActionsSubMenu),
		FUIAction(
			FExecuteAction()
			),
		EUserInterfaceActionType::Button,
		false, 
		FSlateIcon(*StyleSetName, "ContentBrowser.SaveAllCurrentFolder")
		);
}

void FHotPatcherEditorModule::CreateAssetContextMenu(FToolMenuSection& InSection)
{
#if !WITH_UE5
	InSection.AddSubMenu(
		"CookActionsSubMenu",
		LOCTEXT("CookActionsSubMenuLabel", "Cook Actions"),
		LOCTEXT("CookActionsSubMenuToolTip", "Cook actions"),
		FNewToolMenuDelegate::CreateRaw(this, &FHotPatcherEditorModule::MakeCookActionsSubMenu),
		FUIAction(
			FExecuteAction()
			),
		EUserInterfaceActionType::Button,
		false, 
		FSlateIcon(*StyleSetName, "ContentBrowser.AssetActions")
		);
#endif
	InSection.AddSubMenu(
		"CookAndPakActionsSubMenu",
		LOCTEXT("CookAndPakActionsSubMenuLabel", "Cook And Pak Actions"),
		LOCTEXT("CookAndPakActionsSubMenuToolTip", "Cook And Pak actions"),
		FNewToolMenuDelegate::CreateRaw(this, &FHotPatcherEditorModule::MakeCookAndPakActionsSubMenu),
		FUIAction(
			FExecuteAction()
			),
		EUserInterfaceActionType::Button,
		false, 
		FSlateIcon(*StyleSetName, "ContentBrowser.SaveAllCurrentFolder")
		);
	
	// InSection.AddMenuEntry(FHotPatcherCommands::Get().AddToPakSettingsAction);
	InSection.AddDynamicEntry("AddToPatchSettings", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
	{
		const TAttribute<FText> Label = LOCTEXT("CookUtilities_AddToPatchSettings", "Add To Patch Settings");
		const TAttribute<FText> ToolTip = LOCTEXT("CookUtilities_AddToPatchSettingsTooltip", "Add Selected Assets To HotPatcher Patch Settings");
		const FSlateIcon Icon = FSlateIcon(*StyleSetName, "ContentBrowser.AssetActions.Duplicate");
		const FToolMenuExecuteAction UIAction = FToolMenuExecuteAction::CreateRaw(this,&FHotPatcherEditorModule::OnAddToPatchSettings);
	
		InSection.AddMenuEntry("CookUtilities_AddToPatchSettings", Label, ToolTip, Icon, UIAction);
	}));
	
}


void FHotPatcherEditorModule::ExtendContentBrowserAssetSelectionMenu()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("AssetContextCookUtilities"));//Menu->FindOrAddSection("AssetContextReferences");
	FToolMenuEntry& Entry = Section.AddDynamicEntry("AssetManagerEditorViewCommands", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
	{
		UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
		if (Context)
		{
			CreateAssetContextMenu(InSection);
		}
	}));
}

void FHotPatcherEditorModule::ExtendContentBrowserPathSelectionMenu()
{
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 24
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.FolderContextMenu");
	FToolMenuSection& Section = Menu->FindOrAddSection("PathContextCookUtilities");
	FToolMenuEntry& Entry = Section.AddDynamicEntry("AssetManagerEditorViewCommands", FNewToolMenuSectionDelegate::CreateLambda([this](FToolMenuSection& InSection)
	{
		UContentBrowserFolderContext* Context = InSection.FindContext<UContentBrowserFolderContext>();
		if (Context)
		{
			CreateAssetContextMenu(InSection);
		}
	}));
#endif
}

void FHotPatcherEditorModule::MakeCookActionsSubMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection("CookActionsSection");
	for (auto Platform : GetAllowCookPlatforms())
	{
		Section.AddMenuEntry(
            FName(*THotPatcherTemplateHelper::GetEnumNameByValue(Platform)),
            FText::Format(LOCTEXT("Platform", "{0}"), UKismetTextLibrary::Conv_StringToText(THotPatcherTemplateHelper::GetEnumNameByValue(Platform))),
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
	Settings->ReloadConfig();
	for (ETargetPlatform Platform : GetAllowCookPlatforms())
	{
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		FToolMenuEntry& PlatformEntry = Section.AddSubMenu(FName(*PlatformName),
			FText::Format(LOCTEXT("Platform", "{0}"), UKismetTextLibrary::Conv_StringToText(THotPatcherTemplateHelper::GetEnumNameByValue(Platform))),
			FText(),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& SubMenuBuilder){
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("AnalysisDependencies", "AnalysisDependencies"), FText(),
					FSlateIcon(*StyleSetName,TEXT("WorldBrowser.LevelsMenuBrush")),
					FUIAction(FExecuteAction::CreateRaw(this, &FHotPatcherEditorModule::OnCookAndPakPlatform, Platform,true)), NAME_None, EUserInterfaceActionType::Button);
				SubMenuBuilder.AddMenuEntry(
					LOCTEXT("NoDependencies", "NoDependencies"), FText(),
					FSlateIcon(*StyleSetName,TEXT("Level.SaveIcon16x")),
					FUIAction(FExecuteAction::CreateRaw(this, &FHotPatcherEditorModule::OnCookAndPakPlatform, Platform,false)), NAME_None, EUserInterfaceActionType::Button);
			}
			));
	}
}

void FHotPatcherEditorModule::MakeHotPatcherPresetsActionsSubMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection("HotPatcherPresetsActionsSection");
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->ReloadConfig();
	for (FExportPatchSettings PakConfig : Settings->PresetConfigs)
	{
		Section.AddSubMenu(FName(*PakConfig.VersionId),
		FText::Format(LOCTEXT("PakExternal_VersionID", "{0}"), UKismetTextLibrary::Conv_StringToText(PakConfig.VersionId)),
			FText(),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& SubMenuBuilder)
			{
				for (ETargetPlatform Platform : GetAllowCookPlatforms())
				{
					FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
					SubMenuBuilder.AddMenuEntry(
						FText::Format(LOCTEXT("PakPresetPlatform", "{0}"), UKismetTextLibrary::Conv_StringToText(PlatformName)),
						FText(),
						FSlateIcon(*StyleSetName,TEXT("WorldBrowser.LevelsMenuBrush")),
						FUIAction(FExecuteAction::CreateRaw(this, &FHotPatcherEditorModule::OnPakPreset,PakConfig, Platform)), NAME_None, EUserInterfaceActionType::Button);
				}
			}));
	}
}

void FHotPatcherEditorModule::OnAddToPatchSettings(const FToolMenuContext& MenuContent)
{
	OpenDockTab();
	
	TArray<FAssetData> AssetsData = GetSelectedAssetsInBrowserContent();
	TArray<FString> FolderList = GetSelectedFolderInBrowserContent();
	TArray<FDirectoryPath> FolderDirectorys;

	for(const auto& Folder:FolderList)
	{
		FDirectoryPath Path;
		Path.Path = Folder;
		FolderDirectorys.Add(Path);
	}
	TArray<FPatcherSpecifyAsset> AssetsSoftPath;

	for(const auto& AssetData:AssetsData)
	{
		FPatcherSpecifyAsset PatchSettingAssetElement;
		FSoftObjectPath AssetObjectPath;
		AssetObjectPath.SetPath(UFlibAssetManageHelper::GetObjectPathByAssetData(AssetData));
		PatchSettingAssetElement.Asset = AssetObjectPath;
		PatchSettingAssetElement.bAnalysisAssetDependencies = true;
		AssetsSoftPath.AddUnique(PatchSettingAssetElement);
	}
	GPatchSettings->GetAssetScanConfigRef().IncludeSpecifyAssets.Append(AssetsSoftPath);
	GPatchSettings->GetAssetScanConfigRef().AssetIncludeFilters.Append(FolderDirectorys);
}

void FHotPatcherEditorModule::OnPakPreset(FExportPatchSettings Config)
{
	TSharedPtr<FExportPatchSettings> TmpPatchSettings = MakeShareable(new FExportPatchSettings);
	*TmpPatchSettings = Config;
	CookAndPakByPatchSettings(TmpPatchSettings,TmpPatchSettings->IsStandaloneMode());
}

#endif

void FHotPatcherEditorModule::OnCookPlatform(ETargetPlatform Platform)
{
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->ReloadConfig();
	TArray<FAssetData> AssetsDatas = GetSelectedAssetsInBrowserContent();
	TArray<FString> SelectedFolder = GetSelectedFolderInBrowserContent();
	TArray<FAssetData> FolderAssetDatas;
	
	UFlibAssetManageHelper::GetAssetDataInPaths(SelectedFolder,FolderAssetDatas);
	AssetsDatas.Append(FolderAssetDatas);

	TArray<FAssetDetail> AllSelectedAssetDetails;

	for(const auto& AssetData:AssetsDatas)
	{
		AllSelectedAssetDetails.AddUnique(UFlibAssetManageHelper::GetAssetDetailByPackageName(AssetData.PackageName.ToString()));
	}
	
	auto CookNotifyLambda = [](const FString& PackageName,ETargetPlatform Platform,bool bSuccessed)
	{
		FString CookedDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Cooked")));
		// FString PackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(PackagePath);
		FString CookedSavePath = UFlibHotPatcherCoreHelper::GetAssetCookedSavePath(CookedDir,PackageName, THotPatcherTemplateHelper::GetEnumNameByValue(Platform));
		
		auto Msg = FText::Format(
			LOCTEXT("CookAssetsNotify", "Cook {0} for {1} {2}!"),
			UKismetTextLibrary::Conv_StringToText(PackageName),
			UKismetTextLibrary::Conv_StringToText(THotPatcherTemplateHelper::GetEnumNameByValue(Platform)),
			bSuccessed ? UKismetTextLibrary::Conv_StringToText(TEXT("Successfuly")):UKismetTextLibrary::Conv_StringToText(TEXT("Faild"))
			);
		SNotificationItem::ECompletionState CookStatus = bSuccessed ? SNotificationItem::CS_Success:SNotificationItem::CS_Fail;
		UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg,CookedSavePath,CookStatus);
	};
	
	FSingleCookerSettings EmptySetting;
	EmptySetting.MissionID = 0;
	EmptySetting.MissionName = FString::Printf(TEXT("%s_Cooker_%d"),FApp::GetProjectName(),EmptySetting.MissionID);
	EmptySetting.ShaderLibName = FString::Printf(TEXT("%s_Shader_%d"),FApp::GetProjectName(),EmptySetting.MissionID);
	EmptySetting.CookTargetPlatforms = TArray<ETargetPlatform>{Platform};
	EmptySetting.CookAssets = AllSelectedAssetDetails;
	// EmptySetting.ShaderLibName = FApp::GetProjectName();
	EmptySetting.bPackageTracker = false;
	EmptySetting.ShaderOptions.bSharedShaderLibrary = false;
	EmptySetting.IoStoreSettings.bIoStore = false;
	EmptySetting.bSerializeAssetRegistry = false;
	EmptySetting.bDisplayConfig = false;
	EmptySetting.bForceCookInOneFrame = true;
	EmptySetting.StorageCookedDir = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),TEXT("Cooked"));
	EmptySetting.StorageMetadataDir = FPaths::Combine(UFlibHotCookerHelper::GetCookerBaseDir(),EmptySetting.MissionName);
	
	USingleCookerProxy* SingleCookerProxy = NewObject<USingleCookerProxy>();
	SingleCookerProxy->AddToRoot();
	SingleCookerProxy->Init(&EmptySetting);
	SingleCookerProxy->OnAssetCooked.AddLambda([CookNotifyLambda](const FSoftObjectPath& ObjectPath,ETargetPlatform Platform,ESavePackageResult Result)
	{
		CookNotifyLambda(ObjectPath.GetAssetPathString(),Platform,Result == ESavePackageResult::Success);
	});
	
	bool bExportStatus = SingleCookerProxy->DoExport();
	SingleCookerProxy->Shutdown();
}

void FHotPatcherEditorModule::OnPakPreset(FExportPatchSettings Config, ETargetPlatform Platform)
{
	Config.PakTargetPlatforms.Empty();
	Config.PakTargetPlatforms.Add(Platform);
	OnPakPreset(Config);
}

void FHotPatcherEditorModule::CookAndPakByAssetsAndFilters(TArray<FPatcherSpecifyAsset> IncludeAssets,TArray<FDirectoryPath> IncludePaths,TArray<ETargetPlatform> Platforms,bool bForceStandalone)
{
	FString Name = FDateTime::Now().ToString();
	PatchSettings = MakeShareable(new FExportPatchSettings);
	*PatchSettings = MakeTempPatchSettings(Name,IncludePaths,IncludeAssets,TArray<FPlatformExternAssets>{},Platforms,true);
	CookAndPakByPatchSettings(PatchSettings,bForceStandalone);
}


void FHotPatcherEditorModule::CookAndPakByPatchSettings(TSharedPtr<FExportPatchSettings> InPatchSettings,bool bForceStandalone)
{
	if(bForceStandalone || InPatchSettings->IsStandaloneMode())
	{
		FString CurrentConfig;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*InPatchSettings.Get(),CurrentConfig);
		FString SaveConfigTo = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("HotPatcher"),FString::Printf(TEXT("%s_PatchConfig.json"),*InPatchSettings->VersionId)));
		FFileHelper::SaveStringToFile(CurrentConfig,*SaveConfigTo);
		FString MissionCommand = FString::Printf(TEXT("\"%s\" -run=HotPatcher -config=\"%s\" %s"),*UFlibPatchParserHelper::GetProjectFilePath(),*SaveConfigTo,*InPatchSettings->GetCombinedAdditionalCommandletArgs());
		UE_LOG(LogHotPatcher,Log,TEXT("HotPatcher %s Mission: %s %s"),*InPatchSettings->VersionId,*UFlibHotPatcherCoreHelper::GetUECmdBinary(),*MissionCommand);
		
		FText DisplayText = UKismetTextLibrary::Conv_StringToText(FString::Printf(TEXT("Packaging %s for %s..."),*InPatchSettings->VersionId,*UFlibHotPatcherCoreHelper::GetPlatformsStr(InPatchSettings->PakTargetPlatforms)));
		RunProcMission(UFlibHotPatcherCoreHelper::GetUECmdBinary(),MissionCommand,InPatchSettings->VersionId,DisplayText);
	}
	else
	{
		UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
		PatcherProxy->AddToRoot();
		Proxys.Add(PatcherProxy);
		PatcherProxy->Init(InPatchSettings.Get());
		PatcherProxy->DoExport();
	}
}

void FHotPatcherEditorModule::OnCookAndPakPlatform(ETargetPlatform Platform, bool bAnalysicDependencies)
{
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->ReloadConfig();
	
	TArray<FAssetData> AssetsDatas = GetSelectedAssetsInBrowserContent();
	TArray<FString> SelectedFolder = GetSelectedFolderInBrowserContent();
	TArray<FDirectoryPath> IncludePaths;
	TArray<FPatcherSpecifyAsset> IncludeAssets;
	
	for(auto& Folder:SelectedFolder)
	{
		FDirectoryPath CurrentPath;
		if(Folder.StartsWith(TEXT("/All/")))
		{
			Folder.RemoveFromStart(TEXT("/All"));
		}
		CurrentPath.Path = Folder;
		IncludePaths.Add(CurrentPath);
	}

	for(const auto& Asset:AssetsDatas)
	{
		FPatcherSpecifyAsset CurrentAsset;
		CurrentAsset.Asset = Asset.ToSoftObjectPath();
		CurrentAsset.bAnalysisAssetDependencies = bAnalysicDependencies;
		CurrentAsset.AssetRegistryDependencyTypes.AddUnique(EAssetRegistryDependencyTypeEx::Packages);
		IncludeAssets.AddUnique(CurrentAsset);
	}
	CookAndPakByAssetsAndFilters(IncludeAssets,IncludePaths,TArray<ETargetPlatform>{Platform}
#if WITH_UE5
	,true
#endif
	);
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

FExportPatchSettings FHotPatcherEditorModule::MakeTempPatchSettings(
	const FString& Name,
	const TArray<FDirectoryPath>& AssetIncludeFilters,
	const TArray<FPatcherSpecifyAsset>& IncludeSpecifyAssets,
	const TArray<FPlatformExternAssets>& ExternFiles,
	const TArray<ETargetPlatform>& PakTargetPlatforms,
	bool bCook
)
{
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->ReloadConfig();
	FExportPatchSettings TempSettings = Settings->TempPatchSetting;
	TempSettings.VersionId = Name;
	TempSettings.GetAssetScanConfigRef().AssetIncludeFilters = AssetIncludeFilters;
	TempSettings.GetAssetScanConfigRef().IncludeSpecifyAssets = IncludeSpecifyAssets;
	TempSettings.AddExternAssetsToPlatform = ExternFiles;
	TempSettings.bCookPatchAssets = bCook;
	TempSettings.PakTargetPlatforms = PakTargetPlatforms;
	TempSettings.SavePath.Path = Settings->GetTempSavedDir();
	return TempSettings;
}

TArray<ETargetPlatform> FHotPatcherEditorModule::GetAllCookPlatforms() const
{
	TArray<ETargetPlatform> TargetPlatforms;//{ETargetPlatform::Android_ASTC,ETargetPlatform::IOS,ETargetPlatform::WindowsNoEditor};
	#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION > 21
	UEnum* FoundEnum = StaticEnum<ETargetPlatform>();
#else
	FString EnumTypeName = ANSI_TO_TCHAR(THotPatcherTemplateHelper::GetCPPTypeName<ETargetPlatform>().c_str());
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

TArray<ETargetPlatform> FHotPatcherEditorModule::GetAllowCookPlatforms() const
{
	TArray<ETargetPlatform> results;
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->ReloadConfig();
	for (auto Platform : GetAllCookPlatforms())
	{
		if(Settings->bWhiteListCookInEditor && !Settings->PlatformWhitelists.Contains(Platform))
			continue;
		FString PlatformName = THotPatcherTemplateHelper::GetEnumNameByValue(Platform);
		if(PlatformName.StartsWith(TEXT("All")))
			continue;
		results.AddUnique(Platform);
	}
	return results;
}

void FHotPatcherEditorModule::OnCookPlatformForExterner(ETargetPlatform Platform)
{
	FHotPatcherEditorModule::Get().OnCookPlatform(Platform);
}

TSharedPtr<FProcWorkerThread> FHotPatcherEditorModule::RunProcMission(const FString& Bin, const FString& Command, const FString& MissionName, const FText& NotifyTextOverride)
{
	if (mProcWorkingThread.IsValid() && mProcWorkingThread->GetThreadStatus()==EThreadStatus::Busy)
	{
		mProcWorkingThread->Cancel();
	}
	else
	{
		mProcWorkingThread = MakeShareable(new FProcWorkerThread(*FString::Printf(TEXT("PakPresetThread_%s"),*MissionName), Bin, Command));
		mProcWorkingThread->ProcOutputMsgDelegate.BindUObject(MissionNotifyProay,&UMissionNotificationProxy::ReceiveOutputMsg);
		mProcWorkingThread->ProcBeginDelegate.AddUObject(MissionNotifyProay,&UMissionNotificationProxy::SpawnRuningMissionNotification);
		mProcWorkingThread->ProcSuccessedDelegate.AddUObject(MissionNotifyProay,&UMissionNotificationProxy::SpawnMissionSuccessedNotification);
		mProcWorkingThread->ProcFaildDelegate.AddUObject(MissionNotifyProay,&UMissionNotificationProxy::SpawnMissionFaildNotification);
		MissionNotifyProay->SetMissionName(*FString::Printf(TEXT("%s"),*MissionName));
		FText DisplayText = NotifyTextOverride;
		if(DisplayText.IsEmpty())
		{
			DisplayText = FText::FromString(FString::Printf(TEXT("%s in progress"),*MissionName));
		}
		MissionNotifyProay->SetMissionNotifyText(
		DisplayText,
			LOCTEXT("RunningCookNotificationCancelButton", "Cancel"),
			FText::FromString(FString::Printf(TEXT("%s Mission Finished!"),*MissionName)),
			FText::FromString(FString::Printf(TEXT("%s Failed!"),*MissionName))
		);
		MissionNotifyProay->MissionCanceled.AddLambda([this]()
		{
			if (mProcWorkingThread.IsValid() && mProcWorkingThread->GetThreadStatus() == EThreadStatus::Busy)
			{
				mProcWorkingThread->Cancel();
			}
		});
		
		mProcWorkingThread->Execute();
	}
	return mProcWorkingThread;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHotPatcherEditorModule, HotPatcherEditor)