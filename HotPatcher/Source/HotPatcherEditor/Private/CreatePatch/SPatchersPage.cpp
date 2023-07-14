// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CreatePatch/SPatchersPage.h"
#include "CreatePatch/SHotPatcherPatchWidget.h"
#include "CreatePatch/SHotPatcherReleaseWidget.h"

// engine header
#include "HotPatcherEditor.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"

#define LOCTEXT_NAMESPACE "SProjectCreatePatchPage"


/* SProjectCreatePatchPage interface
 *****************************************************************************/

void SPatchersPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherContextBase> InContext)
{
	SetContext(InContext);
	
	// create cook modes menu
	FMenuBuilder PatchModeMenuBuilder(true, NULL);
	TMap<FString,FHotPatcherAction>* Patchers = FHotPatcherActionManager::Get().GetHotPatcherActions().Find(GetPageName());
	if(Patchers)
	{
		for(const auto& Action:*Patchers)
		{
			if(FHotPatcherActionManager::Get().IsSupportEditorAction(Action.Key))
			{
				FUIAction UIAction = FExecuteAction::CreateSP(this, &SPatchersPage::HandleHotPatcherMenuEntryClicked, Action.Value.ActionName.Get().ToString(),Action.Value.ActionCallback);
				PatchModeMenuBuilder.AddMenuEntry(Action.Value.ActionName, Action.Value.ActionToolTip, Action.Value.Icon, UIAction);
			}
		}
	}

	auto Widget = SNew(SVerticalBox)
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
                    .Text(LOCTEXT("ImportProjectConfig", "Import Project Config"))
                    .OnClicked(this,&SPatchersPage::DoImportProjectConfig)
                    .Visibility(this,&SPatchersPage::HandleImportProjectConfigVisibility)
                ]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ImportConfig", "Import"))
					.OnClicked(this,&SPatchersPage::DoImportConfig)
					.Visibility(this,&SPatchersPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportConfig", "Export"))
					.OnClicked(this,&SPatchersPage::DoExportConfig)
					.Visibility(this, &SPatchersPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetConfig", "Reset"))
					.OnClicked(this, &SPatchersPage::DoResetConfig)
					.Visibility(this, &SPatchersPage::HandleOperatorConfigVisibility)
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
						.Text(this, &SPatchersPage::HandlePatchModeComboButtonContentText)
					]
					.ContentPadding(FMargin(6.0, 2.0))
					.MenuContent()
					[
						PatchModeMenuBuilder.MakeWidget()
					]
				]
		];
	
	if(Patchers)
	{
		for(const auto& Patcher:*Patchers)
		{
			if(Patcher.Value.RequestWidget)
			{
				TSharedRef<SHotPatcherWidgetInterface> Action = Patcher.Value.RequestWidget(GetContext());
				Widget->AddSlot().AutoHeight().Padding(0.0, 8.0, 0.0, 0.0)
				[
					Action
				];
				ActionWidgetMap.Add(*Patcher.Key,Action);
			}
		}
	}
	
	ChildSlot
	[
		Widget
	];
	
	if(FHotPatcherAction* DefaultAction = FHotPatcherActionManager::Get().GetTopActionByCategory(GetPageName()))
	{
		HandleHotPatcherMenuEntryClicked(UKismetTextLibrary::Conv_TextToString(DefaultAction->ActionName.Get()),nullptr);
	}
}

EVisibility SPatchersPage::HandleOperatorConfigVisibility()const
{
	return EVisibility::Visible;
}

EVisibility SPatchersPage::HandleImportProjectConfigVisibility() const
{
	TArray<FString> EnableImportActions = {TEXT("ByPatch"),TEXT("ByRelease")};
	bool bEnable = EnableImportActions.Contains(GetContext()->GetModeName().ToString());
	return bEnable ? EVisibility::Visible : EVisibility::Hidden;
}

void SPatchersPage::SelectToAction(const FString& ActionName)
{
	if(FHotPatcherActionManager::Get().IsContainAction(GetPageName(),ActionName))
	{
		HandleHotPatcherMenuEntryClicked(ActionName,nullptr);
	}
}

void SPatchersPage::HandleHotPatcherMenuEntryClicked(FString InModeName,TFunction<void(void)> ActionCallback)
{
	if(ActionCallback)
	{
		ActionCallback();
	}
	if (GetContext().IsValid())
	{
		GetContext()->SetModeByName(FName(*InModeName));
	}
}

FText SPatchersPage::HandlePatchModeComboButtonContentText() const
{
	FText ret;
	if (GetContext().IsValid())
	{
		ret = FText::FromString(GetContext()->GetModeName().ToString());
	}
	return ret;
}

#undef LOCTEXT_NAMESPACE
