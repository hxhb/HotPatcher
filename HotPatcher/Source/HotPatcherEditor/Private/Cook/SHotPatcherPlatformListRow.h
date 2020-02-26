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
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SCheckBox.h"
#include "Model/FHotPatcherCookModel.h"
#include "Templates/SharedPointer.h"

#define LOCTEXT_NAMESPACE "SHotPatcherPlatformListRow"

/**
 * Implements a row widget for map list.
 */
class SHotPatcherPlatformListRow
	: public SMultiColumnTableRow<TSharedPtr<FString> >
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherPlatformListRow) { }
		SLATE_ATTRIBUTE(FString, HighlightString)
		SLATE_ARGUMENT(TSharedPtr<STableViewBase>, OwnerTableView)
		SLATE_ARGUMENT(TSharedPtr<FString>, PlatformName)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 * @param InProfileManager The profile manager to use.
	 */
	void Construct( const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
	{
		HighlightString = InArgs._HighlightString;
		PlatformName = InArgs._PlatformName;

		mCookModel = InCookModel;
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
		if (ColumnName == "PlatformName")
		{
			return SNew(SCheckBox)
				.IsChecked(this, &SHotPatcherPlatformListRow::HandleCheckBoxIsChecked)
				.OnCheckStateChanged(this, &SHotPatcherPlatformListRow::HandleCheckBoxCheckStateChanged)
				.Padding(FMargin(6.0, 2.0))
				[
					SNew(STextBlock)
						.Text(FText::FromString(*PlatformName))
				];
		}

		return SNullWidget::NullWidget;
	}

private:

	// Callback for changing the checked state of the check box.
	void HandleCheckBoxCheckStateChanged( ECheckBoxState NewState )
	{
		if (NewState == ECheckBoxState::Checked)
		{
			mCookModel->AddSelectedCookPlatform(*PlatformName);
		}
		else {
			mCookModel->RemoveSelectedCookPlatform(*PlatformName);
		}
	}

	// Callback for determining the checked state of the check box.
	ECheckBoxState HandleCheckBoxIsChecked( ) const
	{
		if (mCookModel.IsValid())
		{
			if (mCookModel->GetAllSelectedPlatform().Contains(*PlatformName))
			{
				return ECheckBoxState::Checked;
			}
		}
		return ECheckBoxState::Unchecked;
	}

private:

	// Holds the highlight string for the log message.
	TAttribute<FString> HighlightString;

	// Holds the platform's name.
	TSharedPtr<FString> PlatformName;

	TSharedPtr<FHotPatcherCookModel> mCookModel;
};


#undef LOCTEXT_NAMESPACE
