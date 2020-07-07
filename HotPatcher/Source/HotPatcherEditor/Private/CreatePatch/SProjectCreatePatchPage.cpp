// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CreatePatch/SProjectCreatePatchPage.h"
#include "CreatePatch/SHotPatcherExportPatch.h"
#include "CreatePatch/SHotPatcherExportRelease.h"

// engine header
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "SProjectCreatePatchPage"


/* SProjectCreatePatchPage interface
 *****************************************************************************/

void SProjectCreatePatchPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel)
{
	mCreatePatchModel = InCreatePatchModel;

	// create cook modes menu
	FMenuBuilder PatchModeMenuBuilder(true, NULL);
	{
		FUIAction ByReleaseAction(FExecuteAction::CreateSP(this, &SProjectCreatePatchPage::HandleHotPatcherMenuEntryClicked, EHotPatcherActionModes::ByRelease));
		PatchModeMenuBuilder.AddMenuEntry(LOCTEXT("ByExportRelease", "ByRelease"), LOCTEXT("ExportReleaseActionHint", "Export Release ALL Asset Dependencies."), FSlateIcon(), ByReleaseAction);

		FUIAction ByPatchAction(FExecuteAction::CreateSP(this, &SProjectCreatePatchPage::HandleHotPatcherMenuEntryClicked, EHotPatcherActionModes::ByPatch));
		PatchModeMenuBuilder.AddMenuEntry(LOCTEXT("ByCreatePatch", "ByPatch"), LOCTEXT("CreatePatchActionHint", "Create an Patch form Release version."), FSlateIcon(), ByPatchAction);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WhichProjectToUseText", "How would you like to Create Patch the content?"))
			]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ImportConfig", "Import"))
					.OnClicked(this,&SProjectCreatePatchPage::DoImportConfig)
					.Visibility(this,&SProjectCreatePatchPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportConfig", "Export"))
					.OnClicked(this,&SProjectCreatePatchPage::DoExportConfig)
					.Visibility(this, &SProjectCreatePatchPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetConfig", "Reset"))
					.OnClicked(this, &SProjectCreatePatchPage::DoResetConfig)
					.Visibility(this, &SProjectCreatePatchPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					// cooking mode menu
					SNew(SComboButton)
					.ButtonContent()
					[
						SNew(STextBlock)
						.Text(this, &SProjectCreatePatchPage::HandlePatchModeComboButtonContentText)
					]
					.ContentPadding(FMargin(6.0, 2.0))
					.MenuContent()
					[
						PatchModeMenuBuilder.MakeWidget()
					]
				]
		]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SAssignNew(mPatch,SHotPatcherExportPatch, mCreatePatchModel)
				.Visibility(this,&SProjectCreatePatchPage::HandleExportPatchVisibility)
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SAssignNew(mRelease,SHotPatcherExportRelease, mCreatePatchModel)
				.Visibility(this, &SProjectCreatePatchPage::HandleExportReleaseVisibility)
			]
	];

	HandleHotPatcherMenuEntryClicked(EHotPatcherActionModes::ByPatch);
}

FReply SProjectCreatePatchPage::DoImportConfig()const
{
	if (GetActivePatchable().IsValid())
	{
		GetActivePatchable()->ImportConfig();
	}
	return FReply::Handled();
}

FReply SProjectCreatePatchPage::DoExportConfig()const
{
	if (GetActivePatchable().IsValid())
	{
		GetActivePatchable()->ExportConfig();
	}
	return FReply::Handled();
}
FReply SProjectCreatePatchPage::DoResetConfig()const
{
	if (GetActivePatchable().IsValid())
	{
		GetActivePatchable()->ResetConfig();
	}
	return FReply::Handled();
}

TSharedPtr<IPatchableInterface> SProjectCreatePatchPage::GetActivePatchable()const
{
	TSharedPtr<IPatchableInterface> result;
	if (mCreatePatchModel.IsValid())
	{

		switch (mCreatePatchModel->GetPatcherMode())
		{
			case EHotPatcherActionModes::ByPatch:
			{
				result = mPatch;
				break;
			}
			case EHotPatcherActionModes::ByRelease:
			{
				result = mRelease;
				break;
			}
		}
	}
	return result;
}

EVisibility SProjectCreatePatchPage::HandleExportPatchVisibility() const
{
	if (mCreatePatchModel.IsValid())
	{
		if (mCreatePatchModel->GetPatcherMode() == EHotPatcherActionModes::ByPatch)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}
EVisibility SProjectCreatePatchPage::HandleExportReleaseVisibility() const
{
	if (mCreatePatchModel.IsValid())
	{
		if (mCreatePatchModel->GetPatcherMode() == EHotPatcherActionModes::ByRelease)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}
EVisibility SProjectCreatePatchPage::HandleOperatorConfigVisibility()const
{
	return EVisibility::Visible;
}
void SProjectCreatePatchPage::HandleHotPatcherMenuEntryClicked(EHotPatcherActionModes::Type InMode)
{
	if (mCreatePatchModel.IsValid())
	{
		mCreatePatchModel->SetPatcherMode(InMode);
	}
}

FText SProjectCreatePatchPage::HandlePatchModeComboButtonContentText() const
{
	if (mCreatePatchModel.IsValid())
	{
		EHotPatcherActionModes::Type PatcherMode = mCreatePatchModel->GetPatcherMode();

		if (PatcherMode == EHotPatcherActionModes::ByPatch)
		{
			return LOCTEXT("PatcherModeComboButton_ByPatch", "By Patch");
		}
		if (PatcherMode == EHotPatcherActionModes::ByRelease)
		{
			return LOCTEXT("PatcherModeComboButton_ByRelease", "By Release");
		}
	}

	return FText();
}

#undef LOCTEXT_NAMESPACE
