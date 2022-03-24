// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SProjectCookerPage.h"
#include "Cooker/OriginalCooker/SProjectCookPage.h"
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

void SProjectCookerPage::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherModelBase> InCreatePatchModel)
{
	OriginalCookModel = MakeShareable(new FHotPatcherOriginalCookerModel);
	mCreateCookerModel = InCreatePatchModel;

	FHotPatcherEditorModule& EditorModule = FHotPatcherEditorModule::Get();
	
	// create cook modes menu
	FMenuBuilder PatchModeMenuBuilder(true, NULL);
	{
		// FUIAction ByOriginalAction();
		FHotPatcherAction OriginalAction;
		OriginalAction.InLabel = LOCTEXT("ByOriginal", "ByOriginal");
		OriginalAction.InToolTip = LOCTEXT("OriginalCookerActionHint", "Use single-process Cook Content(UE Default)");
		OriginalAction.InIcon = FSlateIcon();
		
		OriginalAction.RequestWidget = [this](TSharedPtr<FHotPatcherModelBase>)->TSharedRef<SCompoundWidget>
		{
			return SNew(SProjectCookPage,OriginalCookModel).Visibility_Lambda([this]()->EVisibility
			{
				return GetCookModelPtr()->GetCookerMode() == EHotPatcherCookActionMode::ByOriginal ? EVisibility::Visible : EVisibility::Collapsed;
			});
		};
		
		FHotPatcherEditorModule::Get().RegisteHotPatcherActions(TEXT("Cooker"),TEXT("ByOriginal"),OriginalAction);

		TMap<FString,FHotPatcherAction>* Cookers = FHotPatcherEditorModule::Get().GetHotPatcherActions().Find(TEXT("Cooker"));
		
		for(const auto& Cooker:*Cookers)
		{
			FUIAction Action = FExecuteAction::CreateSP(this, &SProjectCookerPage::HandleHotPatcherMenuEntryClicked, Cooker.Key,Cooker.Value.ActionCallback);
			PatchModeMenuBuilder.AddMenuEntry(Cooker.Value.InLabel, Cooker.Value.InToolTip, Cooker.Value.InIcon, Action);
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
		];

	TMap<FString,FHotPatcherAction>* Cookers = FHotPatcherEditorModule::Get().GetHotPatcherActions().Find(TEXT("Cooker"));
	if(Cookers)
	{
		for(const auto& Cooker:*Cookers)
		{
			TSharedRef<SCompoundWidget> CookAction = Cooker.Value.RequestWidget(mCreateCookerModel);
		
			Widget->AddSlot().AutoHeight().Padding(0.0, 8.0, 0.0, 0.0)
			[
				CookAction
			];
		}
	}
	ChildSlot
	[
		Widget
	];
	
	HandleHotPatcherMenuEntryClicked(TEXT("ByOriginal"),nullptr);
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
		result = *CookActionMaps.Find(GetCookModelPtr()->GetCookerModeName());
	}
	return result;
}


EVisibility SProjectCookerPage::HandleOperatorConfigVisibility()const
{
	return EVisibility::Visible;
}

EVisibility SProjectCookerPage::HandleImportProjectConfigVisibility() const
{
	EVisibility rVisibility = EVisibility::Hidden;
	if(GetCookModelPtr()->GetCookerMode() != EHotPatcherCookActionMode::ByOriginal)
	{
		rVisibility = EVisibility::Visible;
	}
	return rVisibility;
}

void SProjectCookerPage::HandleHotPatcherMenuEntryClicked(FString InModeName,TFunction<void(void)> ActionCallback)
{
	if(ActionCallback)
	{
		ActionCallback();
	}
	
	if (mCreateCookerModel.IsValid())
	{
		GetCookModelPtr()->SetCookerModeByName(FName(*InModeName));
	}
}

FText SProjectCookerPage::HandleCookerModeComboButtonContentText() const
{
	if (mCreateCookerModel.IsValid())
	{
		return FText::FromString(GetCookModelPtr()->GetCookerModeName());
	}

	return FText();
}

#undef LOCTEXT_NAMESPACE
