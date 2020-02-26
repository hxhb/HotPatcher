// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

// #include "HotPatcherPrivatePCH.h"
#include "SHotPatcherCookMaps.h"
#include "SProjectCookMapListRow.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SSeparator.h"
#include "FlibPatchParserHelper.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "SHotPatcherCookMaps"

void SHotPatcherCookMaps::Construct(const FArguments& InArgs, TSharedPtr<FHotPatcherCookModel> InCookModel)
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
				SAssignNew(MapListView, SListView<TSharedPtr<FString> >)
				.HeaderRow
				(
					SNew(SHeaderRow)
					.Visibility(EVisibility::Collapsed)

					+ SHeaderRow::Column("MapName")
					.DefaultLabel(LOCTEXT("MapListMapNameColumnHeader", "Map"))
					.FillWidth(1.0f)
				)
				.ItemHeight(16.0f)
				.ListItemsSource(&MapList)
				.OnGenerateRow(this, &SHotPatcherCookMaps::HandleMapListViewGenerateRow)
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
							// all Maps hyper-link
							SNew(SHyperlink)
							.OnNavigate(this, &SHotPatcherCookMaps::HandleAllMapHyperlinkNavigate, true)
							.Text(LOCTEXT("AllMapHyperlinkLabel", "All"))
							.ToolTipText(LOCTEXT("AllMapButtonTooltip", "Select all available Maps."))
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							// no Maps hyper-link
							SNew(SHyperlink)
							.OnNavigate(this, &SHotPatcherCookMaps::HandleAllMapHyperlinkNavigate, false)
							.Text(LOCTEXT("NoMapsHyperlinkLabel", "None"))
							.ToolTipText(LOCTEXT("NoMapsHyperlinkTooltip", "Deselect all Maps."))
						]
				]

		];

	RefreshMapList();
}

void SHotPatcherCookMaps::RefreshMapList()
{
	MapList.Reset();


	TArray<FString> AvailableMaps = UFlibPatchParserHelper::GetAvailableMaps(UKismetSystemLibrary::GetProjectDirectory(), false, true);
	for (int32 AvailableMapIndex = 0; AvailableMapIndex < AvailableMaps.Num(); ++AvailableMapIndex)
	{
		FString& Map = AvailableMaps[AvailableMapIndex];
		MapList.Add(MakeShareable(new FString(Map)));
	}

	MapListView->RequestListRefresh();
}

TSharedRef<ITableRow> SHotPatcherCookMaps::HandleMapListViewGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SProjectCookMapListRow, mCookModel.ToSharedRef())
		.MapName(InItem)
		.OwnerTableView(OwnerTable);
}


#undef LOCTEXT_NAMESPACE
