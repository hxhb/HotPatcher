// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Layout/Margin.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Views/SListView.h"

#include "Model/FOriginalCookerContext.h"

#define LOCTEXT_NAMESPACE "SProjectCookSettingsListRow"

/**
 * Implements a row widget for map list.
 */
class SProjectCookSettingsListRow
	: public SMultiColumnTableRow<TSharedPtr<FString> >,public IOriginalCookerChildWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectCookSettingsListRow) { }
		SLATE_ATTRIBUTE(FString, HighlightString)
		SLATE_ARGUMENT(TSharedPtr<STableViewBase>, OwnerTableView)
		SLATE_ARGUMENT(TSharedPtr<FString>, SettingName)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 * @param InProfileManager The profile manager to use.
	 */
	void Construct( const FArguments& InArgs, const TSharedPtr<FHotPatcherContextBase>& InModel )
	{
		HighlightString = InArgs._HighlightString;
		SettingName = InArgs._SettingName;
		SetContext(InModel);

		SMultiColumnTableRow<TSharedPtr<FString> >::Construct(FSuperRowType::FArguments(), InArgs._OwnerTableView.ToSharedRef());
	}

public:

	/**
	 * Generates the widget for the specified column.
	 *
	 * @param ColumnName The name of the column to generate the widget for.
	 * @return The widget.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override
	{
		if (ColumnName == "SettingName")
		{
			return SNew(SCheckBox)
				.IsChecked(this, &SProjectCookSettingsListRow::HandleCheckBoxIsChecked)
				.OnCheckStateChanged(this, &SProjectCookSettingsListRow::HandleCheckBoxCheckStateChanged)
				.Padding(FMargin(6.0, 2.0))
				[
					SNew(STextBlock)
						.Text(FText::FromString(*SettingName))
				];
		}

		return SNullWidget::NullWidget;
	}

private:

	// Callback for changing the checked state of the check box.
	void HandleCheckBoxCheckStateChanged( ECheckBoxState NewState )
	{

		if (mContext.IsValid())
		{
			if (NewState == ECheckBoxState::Checked)
			{
				GetCookerContextPtr()->AddSelectedSetting(*SettingName);
			}
			else
			{
				GetCookerContextPtr()->RemoveSelectedSetting(*SettingName);
			}
		}
	}

	// Callback for determining the checked state of the check box.
	ECheckBoxState HandleCheckBoxIsChecked( ) const
	{

		if (mContext.IsValid() && GetCookerContextPtr()->GetAllSelectedSettings().Contains(*SettingName))
		{
			return ECheckBoxState::Checked;
		}

		return ECheckBoxState::Unchecked;
	}

private:

	// Holds the highlight string for the log message.
	TAttribute<FString> HighlightString;

	// Holds the map's name.
	TSharedPtr<FString> SettingName;
};


#undef LOCTEXT_NAMESPACE
