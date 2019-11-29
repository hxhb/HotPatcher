// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherExportPatch.h"
#include "SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"

#include "FlibHotPatcherEditorHelper.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCreatePatch"

void SHotPatcherExportPatch::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
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
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("DiffVersion", "Diff"))
					.IsEnabled(this, &SHotPatcherExportPatch::CanDiff)
					.OnClicked(this, &SHotPatcherExportPatch::DoDiff)
					.Visibility(this, &SHotPatcherExportPatch::VisibilityDiffButton)
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.Text(LOCTEXT("GeneratePatch", "Generate Patch"))
					.IsEnabled(this, &SHotPatcherExportPatch::CanExportPatch)
					.OnClicked(this, &SHotPatcherExportPatch::DoExportPatch)
				]
			]

		];

	ExportPatchSetting = UExportPatchSettings::Get();
	SettingsView->SetObject(ExportPatchSetting);

}

FReply SHotPatcherExportPatch::DoDiff()const
{


	return FReply::Handled();
}
bool SHotPatcherExportPatch::CanDiff()const
{
	bool bCanDiff = false;
	if (ExportPatchSetting)
	{
		bool bHasBase = !ExportPatchSetting->GetBaseVersion().IsEmpty() && FPaths::FileExists(ExportPatchSetting->GetBaseVersion());
		bool bHasVersionId = !ExportPatchSetting->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportPatchSetting->GetAssetFilters().Num();
		bool bHasSavePath = !ExportPatchSetting->GetSaveAbsPath().IsEmpty();

		bCanDiff = bHasBase && bHasVersionId && bHasFilter && bHasSavePath;
	}
	return bCanDiff;
}

EVisibility SHotPatcherExportPatch::VisibilityDiffButton() const
{
	if (CanExportPatch())
	{
		return EVisibility::Visible;
	}
	else {
		return EVisibility::Collapsed;
	}
}

bool SHotPatcherExportPatch::CanExportPatch()const
{
	bool bCanExport = false;
	if (ExportPatchSetting)
	{
		bool bHasBase = !ExportPatchSetting->GetBaseVersion().IsEmpty() && FPaths::FileExists(ExportPatchSetting->GetBaseVersion());
		bool bHasVersionId = !ExportPatchSetting->GetVersionId().IsEmpty();
		bool bHasFilter = !!ExportPatchSetting->GetAssetFilters().Num();
		bool bHasSavePath = !ExportPatchSetting->GetSaveAbsPath().IsEmpty();
		
		bCanExport = bHasBase && bHasVersionId && bHasFilter && bHasSavePath;
	}
	return bCanExport;
}

FReply SHotPatcherExportPatch::DoExportPatch()
{

	return FReply::Handled();
}

void SHotPatcherExportPatch::CreateExportFilterListView()
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


#undef LOCTEXT_NAMESPACE
