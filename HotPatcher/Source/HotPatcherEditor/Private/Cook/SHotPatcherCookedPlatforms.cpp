// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherCookedPlatforms.h"
#include "SHotPatcherPlatformListRow.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCookedPlatforms"

void SHotPatcherCookedPlatforms::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
{

	mCookModel = InCookModel;
	MakePlatformMenu();

	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.MaxHeight(256.0f)
			[
				// platform menu
				SAssignNew(PlatformListView, SListView<TSharedPtr<FString> >)
				.HeaderRow(
				SNew(SHeaderRow)
				.Visibility(EVisibility::Collapsed)

				+ SHeaderRow::Column("PlatformName")
				.DefaultLabel(LOCTEXT("PlatformListPlatformNameColumnHeader", "Platform"))
				.FillWidth(1.0f)
				)
				.ItemHeight(16.0f)
				.ListItemsSource(&PlatformList)
				.OnGenerateRow(this, &SHotPatcherCookedPlatforms::HandlePlatformListViewGenerateRow)
				.SelectionMode(ESelectionMode::None)
			]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f, 0.0f, 4.0f)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectLabel", "Select:"))
					]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(8.0f, 0.0f)
						[
							// all platforms hyper-link
							SNew(SHyperlink)
							.OnNavigate(this, &SHotPatcherCookedPlatforms::HandleAllPlatformsHyperlinkNavigate, true)
							.Text(LOCTEXT("AllPlatformsHyperlinkLabel", "All"))
							.ToolTipText(LOCTEXT("AllPlatformsButtonTooltip", "Select all available platforms."))
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							// no platforms hyper-link
							SNew(SHyperlink)
							.OnNavigate(this, &SHotPatcherCookedPlatforms::HandleAllPlatformsHyperlinkNavigate, false)
							.Text(LOCTEXT("NoPlatformsHyperlinkLabel", "None"))
							.ToolTipText(LOCTEXT("NoPlatformsHyperlinkTooltip", "Deselect all platforms."))
						]
				]
		];
}

TSharedRef<ITableRow> SHotPatcherCookedPlatforms::HandlePlatformListViewGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SHotPatcherPlatformListRow,mCookModel)
		.PlatformName(InItem)
		.OwnerTableView(OwnerTable);
}

#undef LOCTEXT_NAMESPACE
