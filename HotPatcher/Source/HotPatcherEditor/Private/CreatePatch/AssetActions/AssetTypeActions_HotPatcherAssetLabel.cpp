#include "AssetTypeActions_HotPatcherAssetLabel.h"
#include "Engine/PrimaryAssetLabel.h"
#include "HotPatcherEditor.h"
#include "FlibAssetManageHelper.h"

#if WITH_EDITOR_SECTION
#include "ToolMenuSection.h"
void FAssetTypeActions_HotPatcherAssetLabel::GetActions(const TArray<UObject*>& InObjects,
	FToolMenuSection& Section)
{
	auto Labels = GetTypedWeakObjectPtrs<UHotPatcherPrimaryLabel>(InObjects);
	const FSlateIcon Icon = FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Duplicate");

	Section.AddMenuEntry(
			"ObjectContext_AddToChunkConfig",
			NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToChunkConfig", "Add To Chunk Config"),
			NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToChunkConfigTooltip", "Add Label To Chunk Config"),
			Icon,
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_HotPatcherAssetLabel::ExecuteAddToChunkConfig, Labels)
			));
	Section.AddSubMenu(
		"ObjectContext_CookAndPakActionsSubMenu",
		NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_CookAndPakActionsSubMenu","Cook And Pak Label Actions"),
		NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_CookAndPakActionsSubMenu","Cook And Pak Label Actions"),
		FNewToolMenuDelegate::CreateRaw(this, &FAssetTypeActions_HotPatcherAssetLabel::MakeCookAndPakActionsSubMenu,Labels),
		FUIAction(
			FExecuteAction()
			),
		EUserInterfaceActionType::Button,
		false, 
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.SaveAllCurrentFolder")
		);
	
}



TArray<FChunkInfo> GetChunksByAssetLabels(TArray<TWeakObjectPtr<UHotPatcherPrimaryLabel>> Objects)
{
	TArray<FChunkInfo> Chunks;

	for(const auto& Object:Objects)
	{
		FChunkInfo Chunk = Object->LabelRules;
		Chunks.Add(Chunk);
	}
	return Chunks;
}

void FAssetTypeActions_HotPatcherAssetLabel::ExecuteAddToChunkConfig(TArray<TWeakObjectPtr<UHotPatcherPrimaryLabel>> Objects)
{
	FHotPatcherEditorModule::Get().OpenDockTab();
	if(GPatchSettings)
	{
		GPatchSettings->bEnableChunk = true;
		GPatchSettings->ChunkInfos.Append(GetChunksByAssetLabels(Objects));
	}
}

void FAssetTypeActions_HotPatcherAssetLabel::MakeCookAndPakActionsSubMenu(UToolMenu* Menu,TArray<TWeakObjectPtr<UHotPatcherPrimaryLabel>> Objects)
{
	FToolMenuSection& Section = Menu->AddSection("CookAndPakActionsSection");
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->ReloadConfig();
	for (auto Platform : FHotPatcherEditorModule::Get().GetAllCookPlatforms())
	{
		if(Settings->bWhiteListCookInEditor && !Settings->PlatformWhitelists.Contains(Platform))
			continue;
		Section.AddMenuEntry(
			FName(*THotPatcherTemplateHelper::GetEnumNameByValue(Platform)),
			FText::Format(NSLOCTEXT("Platform","Platform", "{0}"), UKismetTextLibrary::Conv_StringToText(THotPatcherTemplateHelper::GetEnumNameByValue(Platform))),
			FText(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FAssetTypeActions_HotPatcherAssetLabel::OnCookAndPakPlatform, Platform,Objects)
			)
		);
	}
}

void FAssetTypeActions_HotPatcherAssetLabel::OnCookAndPakPlatform(ETargetPlatform Platform,TArray<TWeakObjectPtr<UHotPatcherPrimaryLabel>> Objects)
{
	TArray<ETargetPlatform> PakTargetPlatforms;
	for(const auto& Chunk:GetChunksByAssetLabels(Objects))
	{
		TSharedPtr<FExportPatchSettings> PatchSettings = UFlibHotPatcherCoreHelper::MakePatcherSettingByChunk(Chunk,PakTargetPlatforms);
		PatchSettings->PakTargetPlatforms = PakTargetPlatforms;
		FHotPatcherEditorModule::Get().CookAndPakByPatchSettings(PatchSettings,false);
	}
}

#endif
