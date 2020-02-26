// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherCookSetting.h"
#include "SProjectCookSettingsListRow.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"

#include "FlibHotPatcherEditorHelper.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCookSetting"

void SHotPatcherCookSetting::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
{

	mCookModel = InCookModel;

	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0)
			.Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				// map list
				SAssignNew(SettingListView, SListView<TSharedPtr<FString> >)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.Visibility(EVisibility::Collapsed)

					+ SHeaderRow::Column("SettingName")
					.DefaultLabel(LOCTEXT("SettingListSettingNameColumnHeader", "Setting"))
					.FillWidth(1.0f)
				)
				.ItemHeight(16.0f)
				.ListItemsSource(&SettingList)
				.OnGenerateRow(this, &SHotPatcherCookSetting::HandleCookSettingListViewGenerateRow)
				.SelectionMode(ESelectionMode::None)
			]
			+ SVerticalBox::Slot()	
			.AutoHeight()
			.Padding(0.0f,2.0f,0.0f,0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ProjectCookSettingExParama", "Other Options:"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 2.0f, 0.0f, 0.0f)
				.FillWidth(1.0f)
				[
					SAssignNew(ExternSettingTextBox, SEditableTextBox)
				]
			]
		];

	mCookModel->OnRequestExSettings.BindRaw(this, &SHotPatcherCookSetting::HandleRequestExSettings);
	RefreshSettingsList();
}

TSharedRef<ITableRow> SHotPatcherCookSetting::HandleCookSettingListViewGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SProjectCookSettingsListRow, mCookModel.ToSharedRef())
		.SettingName(InItem)
		.OwnerTableView(OwnerTable);
}

void SHotPatcherCookSetting::RefreshSettingsList()
{
	SettingList.Reset();


	TArray<FString> AllSettings = UFlibHotPatcherEditorHelper::GetAllCookOption();
	for (int32 OptionIndex = 0; OptionIndex < AllSettings.Num(); ++OptionIndex)
	{
		FString& Option = AllSettings[OptionIndex];
		SettingList.Add(MakeShareable(new FString(Option)));
	}

	SettingListView->RequestListRefresh();
}

void SHotPatcherCookSetting::HandleRequestExSettings(TArray<FString>& OutExSettings)
{
	OutExSettings.Reset();

	FString ExOptins = ExternSettingTextBox->GetText().ToString();
	OutExSettings.Add(ExOptins);
}

#undef LOCTEXT_NAMESPACE
