#include "AssetTypeActions_PrimaryAssetLabel.h"
#include "Engine/PrimaryAssetLabel.h"
#include "HotPatcherEditor.h"
#include "FlibAssetManageHelper.h"
#include "Misc/EngineVersionComparison.h"

#if !UE_VERSION_OLDER_THAN(5,1,0)
	typedef FAppStyle FEditorStyle;
#endif

#if WITH_EDITOR_SECTION
#include "ToolMenuSection.h"
void FAssetTypeActions_PrimaryAssetLabel::GetActions(const TArray<UObject*>& InObjects,
	FToolMenuSection& Section)
{
#if !UE_VERSION_OLDER_THAN(5,1,0)
	FName StyleSetName = FEditorStyle::GetAppStyleSetName();
#else
	FName StyleSetName = FEditorStyle::GetStyleSetName();
#endif
	auto Labels = GetTypedWeakObjectPtrs<UPrimaryAssetLabel>(InObjects);
	const FSlateIcon Icon = FSlateIcon(StyleSetName, "ContentBrowser.AssetActions.Duplicate");
	Section.AddMenuEntry(
	"ObjectContext_AddToPatchIncludeFilters",
	NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToPatchIncludeFilters", "Add To Patch Include Filters"),
	NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToPatchIncludeFiltersTooltip", "Add the label to HotPatcher Include Filters."),
	Icon,
	FUIAction(
		FExecuteAction::CreateSP(this, &FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToPatchIncludeFilter, Labels)
	));
	Section.AddMenuEntry(
			"ObjectContext_AddToChunkConfig",
			NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToChunkConfig", "Add To Chunk Config"),
			NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_AddToChunkConfigTooltip", "Add Label To Chunk Config"),
			Icon,
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToChunkConfig, Labels)
			));
	Section.AddSubMenu(
		"ObjectContext_CookAndPakActionsSubMenu",
		NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_CookAndPakActionsSubMenu","Cook And Pak Label Actions"),
		NSLOCTEXT("AssetTypeActions_PrimaryAssetLabel", "ObjectContext_CookAndPakActionsSubMenu","Cook And Pak Label Actions"),
		FNewToolMenuDelegate::CreateRaw(this, &FAssetTypeActions_PrimaryAssetLabel::MakeCookAndPakActionsSubMenu,Labels),
		FUIAction(
			FExecuteAction()
			),
		EUserInterfaceActionType::Button,
		false, 
		FSlateIcon(StyleSetName, "ContentBrowser.SaveAllCurrentFolder")
		);
	
}
TArray<FPatcherSpecifyAsset> GetLabelsAssets(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	TArray<FPatcherSpecifyAsset> LabelAsstes;
	for(auto& Label:Objects)
	{
		if( Label->Rules.CookRule == EPrimaryAssetCookRule::NeverCook )
			continue;
		for(const auto& Asset:Label->ExplicitAssets)
		{
			FPatcherSpecifyAsset CurrentAsset;
			CurrentAsset.Asset = Asset.ToSoftObjectPath();
			CurrentAsset.bAnalysisAssetDependencies = Label->bLabelAssetsInMyDirectory;
			CurrentAsset.AssetRegistryDependencyTypes.AddUnique(EAssetRegistryDependencyTypeEx::Packages);
			LabelAsstes.AddUnique(CurrentAsset);
		}
	}
	return LabelAsstes;
}

TArray<FDirectoryPath> GetLabelsDirs(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	TArray<FDirectoryPath> Dirs;
	for(auto& Label:Objects)
	{
		if(Label->bLabelAssetsInMyDirectory && (Label->Rules.CookRule != EPrimaryAssetCookRule::NeverCook))
		{
			FString PathName = Label->GetPathName();
			TArray<FString> DirNames;
			PathName.ParseIntoArray(DirNames,TEXT("/"));
			FString FinalPath;
			for(size_t index = 0;index < DirNames.Num() - 1;++index)
			{
				FinalPath += TEXT("/") + DirNames[index];
			}
			FDirectoryPath CurrentDir;
			CurrentDir.Path = FinalPath;
			Dirs.Add(CurrentDir);
		}
	}
	return Dirs;
}

void FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToPatchIncludeFilter(
	TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	FHotPatcherEditorModule::Get().OpenDockTab();
	if(GPatchSettings)
	{
		GPatchSettings->GetAssetScanConfigRef().IncludeSpecifyAssets.Append(GetLabelsAssets(Objects));
		GPatchSettings->GetAssetScanConfigRef().AssetIncludeFilters.Append(GetLabelsDirs(Objects));
	}
}

TArray<FChunkInfo> GetChunksByAssetLabels(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	TArray<FChunkInfo> Chunks;

	for(const auto& Object:Objects)
	{
		FChunkInfo Chunk;
		Chunk.AssetIgnoreFilters = GetLabelsDirs(TArray<TWeakObjectPtr<UPrimaryAssetLabel>>{Object});
		Chunk.IncludeSpecifyAssets = GetLabelsAssets(TArray<TWeakObjectPtr<UPrimaryAssetLabel>>{Object});
		Chunk.bAnalysisFilterDependencies = Object->Rules.bApplyRecursively;
		FString LongPackageName = UFlibAssetManageHelper::PackagePathToLongPackageName(Object->GetPathName());
		{
			TArray<FString> DirNames;
			LongPackageName.ParseIntoArray(DirNames,TEXT("/"));
			Chunk.ChunkName = DirNames[DirNames.Num()-1];
		}
	}
	return Chunks;
}

void FAssetTypeActions_PrimaryAssetLabel::ExecuteAddToChunkConfig(TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	FHotPatcherEditorModule::Get().OpenDockTab();
	if(GPatchSettings)
	{
		GPatchSettings->bEnableChunk = true;
		GPatchSettings->ChunkInfos.Append(GetChunksByAssetLabels(Objects));
	}
}

void FAssetTypeActions_PrimaryAssetLabel::MakeCookAndPakActionsSubMenu(UToolMenu* Menu,TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	FToolMenuSection& Section = Menu->AddSection("CookAndPakActionsSection");
	UHotPatcherSettings* Settings = GetMutableDefault<UHotPatcherSettings>();
	Settings->ReloadConfig();
	for (auto Platform : FHotPatcherEditorModule::Get().GetAllowCookPlatforms())
	{
		if(Settings->bWhiteListCookInEditor && !Settings->PlatformWhitelists.Contains(Platform))
			continue;
		Section.AddMenuEntry(
			FName(*THotPatcherTemplateHelper::GetEnumNameByValue(Platform)),
			FText::Format(NSLOCTEXT("Platform","Platform", "{0}"), UKismetTextLibrary::Conv_StringToText(THotPatcherTemplateHelper::GetEnumNameByValue(Platform))),
			FText(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FAssetTypeActions_PrimaryAssetLabel::OnCookAndPakPlatform, Platform,Objects)
			)
		);
	}
}

void FAssetTypeActions_PrimaryAssetLabel::OnCookAndPakPlatform(ETargetPlatform Platform,TArray<TWeakObjectPtr<UPrimaryAssetLabel>> Objects)
{
	FHotPatcherEditorModule::Get().CookAndPakByAssetsAndFilters(GetLabelsAssets(Objects),GetLabelsDirs(Objects),TArray<ETargetPlatform>{Platform},true);
}

#endif
