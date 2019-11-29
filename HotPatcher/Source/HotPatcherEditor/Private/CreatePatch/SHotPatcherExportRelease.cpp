// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherExportRelease.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FLibAssetManageHelperEx.h"
#include "AssetManager/FAssetDependenciesInfo.h"

// engine header
#include "SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"

#define LOCTEXT_NAMESPACE "SHotPatcherExportRelease"

void SHotPatcherExportRelease::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
{
	CreateExportFilterListView();

	mCreatePatchModel = InCreatePatchModel;

	ChildSlot
		[

			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SettingsView->AsShared()
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(4, 4, 10, 4)
			[
				SNew(SButton)
				.Text(LOCTEXT("GenerateRelease", "Export Release"))
				.OnClicked(this,&SHotPatcherExportRelease::DoExportRelease)
				.IsEnabled(this,&SHotPatcherExportRelease::CanExportRelease)
			]
		];

	ExportReleaseSettings = UExportReleaseSettings::Get();
	SettingsView->SetObject(ExportReleaseSettings);
}

void SHotPatcherExportRelease::CreateExportFilterListView()
{
	// Create a property view
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = true;
	DetailsViewArgs.bLockable = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	DetailsViewArgs.bCustomNameAreaLocation = false;
	DetailsViewArgs.bCustomFilterAreaLocation = true;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

	SettingsView = EditModule.CreateDetailView(DetailsViewArgs);
}

bool SHotPatcherExportRelease::CanExportRelease()const
{
	bool bCanExport=false;
	if (ExportReleaseSettings)
	{
		bool bHasVersion = !ExportReleaseSettings->GetVersionId().IsEmpty();
		bool bHasFilter = (ExportReleaseSettings->GetAssetFilters().Num() > 0);
		bool bHasSavePath = !(ExportReleaseSettings->GetSavePath().IsEmpty());

		bCanExport = bHasVersion && bHasFilter&&bHasSavePath;
	}
	return bCanExport;
}

FReply SHotPatcherExportRelease::DoExportRelease()
{
	//TArray<FString> AllAssets;
	//TArray<FString> AllFilter;
	//for (const auto& Filter : ExportReleaseSettings->GetAssetFilters())
	//{
	//	AllFilter.AddUnique(Filter);
	//}

	//UFLibAssetManageHelperEx::GetAssetsList(AllFilter, AllAssets);
	//if (AllAssets.Num())
	//{
	//	FAssetDependenciesInfo AssetDependencies;
	//	UFLibAssetManageHelperEx::GetAssetListDependencies(AllAssets,AssetDependencies);
	//	FString SaveToJson;
	//	if (UFLibAssetManageHelperEx::SerializeAssetDependenciesToJson(AssetDependencies, SaveToJson))
	//	{
	//		bool runState = UFLibAssetManageHelperEx::SaveStringToFile(FPaths::Combine(ExportReleaseSettings->GetSavePath(), ExportReleaseSettings->GetVersionId() + TEXT(".json")), SaveToJson);
	//		UE_LOG(LogTemp, Log, TEXT("HotPatcher Export RELEASE is %s."), runState ? TEXT("Success") : TEXT("FAILD"));
	//	}
	//}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
