// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SProjectCookerPage.h"
#include "Cooker/OriginalCooker/SProjectCookPage.h"
#include "Cooker/MultiCooker/SHotPatcherMultiCookerPage.h"
// engine header
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "SProjectCookerPage"


/* SProjectCookerPage interface
 *****************************************************************************/

void SProjectCookerPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookerModel> InCreatePatchModel)
{
	OriginalCookModel = MakeShareable(new FHotPatcherOriginalCookerModel);
	mCreateCookerModel = InCreatePatchModel;

	// create cook modes menu
	FMenuBuilder PatchModeMenuBuilder(true, NULL);
	{
		FUIAction ByOriginalAction(FExecuteAction::CreateSP(this, &SProjectCookerPage::HandleHotPatcherMenuEntryClicked, EHotPatcherCookActionMode::ByOriginal));
		PatchModeMenuBuilder.AddMenuEntry(LOCTEXT("ByOriginal", "ByOriginal"), LOCTEXT("OriginalCookerActionHint", "Use single-process Cook Content(UE Default)"), FSlateIcon(), ByOriginalAction);
#if ENABLE_MULTI_COOKER
		FUIAction ByMultiProcessAction(FExecuteAction::CreateSP(this, &SProjectCookerPage::HandleHotPatcherMenuEntryClicked, EHotPatcherCookActionMode::ByMultiProcess));
		PatchModeMenuBuilder.AddMenuEntry(LOCTEXT("ByMultiProcess", "ByMultiProcess"), LOCTEXT("MultiCookerActionHint", "Use multi-process Cook Content"), FSlateIcon(), ByMultiProcessAction);
#endif
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
				.Text(LOCTEXT("WhichProjectToUseText", "How would you like to Cook the content?"))
			]
			+ SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0, 0.0, 0.0, 0.0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ImportProjectConfig", "Import Project Config"))
                    .OnClicked(this,&SProjectCookerPage::DoImportProjectConfig)
                    .Visibility(this,&SProjectCookerPage::HandleImportProjectConfigVisibility)
                ]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ImportConfig", "Import"))
					.OnClicked(this,&SProjectCookerPage::DoImportConfig)
					.Visibility(this,&SProjectCookerPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportConfig", "Export"))
					.OnClicked(this,&SProjectCookerPage::DoExportConfig)
					.Visibility(this, &SProjectCookerPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetConfig", "Reset"))
					.OnClicked(this, &SProjectCookerPage::DoResetConfig)
					.Visibility(this, &SProjectCookerPage::HandleOperatorConfigVisibility)
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
						.Text(this, &SProjectCookerPage::HandleCookerModeComboButtonContentText)
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
				SAssignNew(mOriginal,SProjectCookPage,OriginalCookModel)
				.Visibility(this,&SProjectCookerPage::HandleOriginalCookerVisibility)
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SAssignNew(mMultiProcess,SHotPatcherMultiCookerPage,mCreateCookerModel)
				.Visibility(this, &SProjectCookerPage::HandleMultiProcessCookerVisibility)
			]
	];

	HandleHotPatcherMenuEntryClicked(EHotPatcherCookActionMode::ByOriginal);
}

FReply SProjectCookerPage::DoImportConfig()const
{
	if (GetActivePatchable().IsValid())
	{
		GetActivePatchable()->ImportConfig();
	}
	return FReply::Handled();
}

FReply SProjectCookerPage::DoImportProjectConfig() const
{
	if (GetActivePatchable().IsValid())
	{
		GetActivePatchable()->ImportProjectConfig();
	}
	return FReply::Handled();
}

FReply SProjectCookerPage::DoExportConfig()const
{
	if (GetActivePatchable().IsValid())
	{
		GetActivePatchable()->ExportConfig();
	}
	return FReply::Handled();
}
FReply SProjectCookerPage::DoResetConfig()const
{
	if (GetActivePatchable().IsValid())
	{
		GetActivePatchable()->ResetConfig();
	}
	return FReply::Handled();
}

TSharedPtr<IPatchableInterface> SProjectCookerPage::GetActivePatchable()const
{
	TSharedPtr<IPatchableInterface> result;
	if (mCreateCookerModel.IsValid())
	{

		switch (mCreateCookerModel->GetCookerMode())
		{
			case EHotPatcherCookActionMode::ByOriginal:
			{
				result = mOriginal;
				break;
			}
			case EHotPatcherCookActionMode::ByMultiProcess:
			{
				result = mMultiProcess;
				break;
			}
		}
	}
	return result;
}

EVisibility SProjectCookerPage::HandleOriginalCookerVisibility() const
{
	if (mCreateCookerModel.IsValid())
	{
		if (mCreateCookerModel->GetCookerMode() == EHotPatcherCookActionMode::ByOriginal)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}
EVisibility SProjectCookerPage::HandleMultiProcessCookerVisibility() const
{
	if (mCreateCookerModel.IsValid())
	{
		if (mCreateCookerModel->GetCookerMode() == EHotPatcherCookActionMode::ByMultiProcess)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SProjectCookerPage::HandleOperatorConfigVisibility()const
{
	return EVisibility::Visible;
}

EVisibility SProjectCookerPage::HandleImportProjectConfigVisibility() const
{
	switch (mCreateCookerModel->GetCookerMode())
	{
		case EHotPatcherCookActionMode::ByMultiProcess:
		{
			return EVisibility::Visible;
		}
		case EHotPatcherCookActionMode::ByOriginal:
			{
				return EVisibility::Hidden;
			}
		default:
			return EVisibility::Hidden;
	}
}

void SProjectCookerPage::HandleHotPatcherMenuEntryClicked(EHotPatcherCookActionMode::Type InMode)
{
	if (mCreateCookerModel.IsValid())
	{
		mCreateCookerModel->SetCookerMode(InMode);
	}
}

FText SProjectCookerPage::HandleCookerModeComboButtonContentText() const
{
	if (mCreateCookerModel.IsValid())
	{
		EHotPatcherCookActionMode::Type PatcherMode = mCreateCookerModel->GetCookerMode();

		if (PatcherMode == EHotPatcherCookActionMode::ByOriginal)
		{
			return LOCTEXT("CookerModeComboButtoncess_ByOriginal", "By Original");
		}
		if (PatcherMode == EHotPatcherCookActionMode::ByMultiProcess)
		{
			return LOCTEXT("CookerModeComboButtoncess_ByMultiProcess", "By MultiProcess");
		}
	}

	return FText();
}

#undef LOCTEXT_NAMESPACE
