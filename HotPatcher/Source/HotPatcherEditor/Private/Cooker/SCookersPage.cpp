// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCookersPage.h"
#include "Cooker/OriginalCooker/SOriginalCookWidget.h"
// #include "Cooker/MultiCooker/SHotPatcherMultiCookerPage.h"
// engine header
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "HotPatcherEditor.h"
#define LOCTEXT_NAMESPACE "SProjectCookerPage"


/* SProjectCookerPage interface
 *****************************************************************************/

void SCookersPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherContextBase> InContext)
{
	SetContext(InContext);
	FHotPatcherEditorModule& EditorModule = FHotPatcherEditorModule::Get();
	
	// create cook modes menu
	FMenuBuilder PatchModeMenuBuilder(true, NULL);
	{
		TMap<FString,FHotPatcherAction>* Cookers = FHotPatcherActionManager::Get().GetHotPatcherActions().Find(GetPageName());
		if(Cookers)
		{
			for(const auto& Cooker:*Cookers)
			{
				if(FHotPatcherActionManager::Get().IsSupportEditorAction(Cooker.Key))
				{
					FUIAction Action = FExecuteAction::CreateSP(this, &SCookersPage::HandleHotPatcherMenuEntryClicked, Cooker.Key,Cooker.Value.ActionCallback);
					PatchModeMenuBuilder.AddMenuEntry(Cooker.Value.ActionName, Cooker.Value.ActionToolTip, Cooker.Value.Icon, Action);
				}
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
				.Text(LOCTEXT("WhichProjectToUseText", "How would you like to Cook the content?"))
			]
			+ SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0, 0.0, 0.0, 0.0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ImportProjectConfig", "Import Project Config"))
                    .OnClicked(this,&SHotPatcherPageBase::DoImportProjectConfig)
                    .Visibility(this,&SCookersPage::HandleImportProjectConfigVisibility)
                ]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ImportConfig", "Import"))
					.OnClicked(this,&SHotPatcherPageBase::DoImportConfig)
					.Visibility(this,&SCookersPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportConfig", "Export"))
					.OnClicked(this,&SHotPatcherPageBase::DoExportConfig)
					.Visibility(this, &SCookersPage::HandleOperatorConfigVisibility)
				]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.0, 0.0, 0.0, 0.0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetConfig", "Reset"))
					.OnClicked(this, &SHotPatcherPageBase::DoResetConfig)
					.Visibility(this, &SCookersPage::HandleOperatorConfigVisibility)
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
						.Text(this, &SCookersPage::HandleCookerModeComboButtonContentText)
					]
					.ContentPadding(FMargin(6.0, 2.0))
					.MenuContent()
					[
						PatchModeMenuBuilder.MakeWidget()
					]
				]
		];

	TMap<FString,FHotPatcherAction>* Cookers = FHotPatcherActionManager::Get().GetHotPatcherActions().Find(GetPageName());
	if(Cookers)
	{
		for(const auto& Cooker:*Cookers)
		{
			if(Cooker.Value.RequestWidget)
			{
				TSharedRef<SHotPatcherWidgetInterface> Action = Cooker.Value.RequestWidget(GetContext());
		
				Widget->AddSlot().AutoHeight().Padding(0.0, 8.0, 0.0, 0.0)
				[
					Action
				];
				ActionWidgetMap.Add(*Cooker.Key,Action);
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

void SCookersPage::SelectToAction(const FString& ActionName)
{
	if(FHotPatcherActionManager::Get().IsContainAction(GetPageName(),ActionName))
	{
		HandleHotPatcherMenuEntryClicked(ActionName,nullptr);
	}
}

EVisibility SCookersPage::HandleOperatorConfigVisibility()const
{
	return EVisibility::Visible;
}

EVisibility SCookersPage::HandleImportProjectConfigVisibility() const
{
	EVisibility rVisibility = EVisibility::Hidden;
	if(GetContext()->GetModeName().IsEqual(TEXT("ByOriginal")))
	{
		rVisibility = EVisibility::Visible;
	}
	return rVisibility;
}

void SCookersPage::HandleHotPatcherMenuEntryClicked(FString InModeName,TFunction<void(void)> ActionCallback)
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

FText SCookersPage::HandleCookerModeComboButtonContentText() const
{
	if (GetContext().IsValid())
	{
		return FText::FromString(GetContext()->GetModeName().ToString());
	}

	return FText();
}

#undef LOCTEXT_NAMESPACE
