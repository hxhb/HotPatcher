// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherCookSetting.h"
#include "SProjectCookSettingsListRow.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "Kismet/KismetTextLibrary.h"

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

TSharedPtr<FJsonObject> SHotPatcherCookSetting::SerializeAsJson() const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	TArray<FString> SelectedCookSettingList = mCookModel->GetAllSelectedSettings();

	TArray<TSharedPtr<FJsonValue>> CookSettingJsonList;
	for (const auto& Platform : SelectedCookSettingList)
	{
		CookSettingJsonList.Add(MakeShareable(new FJsonValueString(Platform)));
	}
	
	JsonObject->SetArrayField(TEXT("CookSettings"), CookSettingJsonList);
	FString FinalOptions = ExternSettingTextBox->GetText().ToString();

	for (const auto& DefaultCookParam : GetDefaultCookParams())
	{
		if (!FinalOptions.Contains(DefaultCookParam))
		{
			FinalOptions.Append(TEXT(" ") + DefaultCookParam);
		}
	}
	JsonObject->SetStringField(TEXT("Options"), FinalOptions);
	return JsonObject;
}

void SHotPatcherCookSetting::DeSerializeFromJsonObj(TSharedPtr<FJsonObject>const & InJsonObject)
{
	TArray<TSharedPtr<FJsonValue>> CookSettingJsonList = InJsonObject->GetArrayField(TEXT("CookSettings"));

	TArray<TSharedPtr<FString>> SelectedCookSettingList;
	for (const auto& SettingItem : CookSettingJsonList)
	{
		FString Setting = SettingItem->AsString();
		SelectedCookSettingList.Add(MakeShareable(new FString(Setting)));
		mCookModel->AddSelectedSetting(Setting);
	}
	ExternSettingTextBox->SetText(UKismetTextLibrary::Conv_StringToText(InJsonObject->GetStringField("Options")));
}


FString SHotPatcherCookSetting::GetSerializeName()const
{
	return TEXT("Settings");
}

void SHotPatcherCookSetting::Reset()
{
	mCookModel->ClearAllSettings();
	ExternSettingTextBox->SetText(UKismetTextLibrary::Conv_StringToText(TEXT("")));
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
