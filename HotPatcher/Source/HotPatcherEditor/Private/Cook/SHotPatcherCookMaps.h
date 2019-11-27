// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "FHotPatcherCookModel.h"
#include "SharedPointer.h"
/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherCookMaps
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherCookMaps) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookModel> InCookModel);

protected:

	TSharedRef<ITableRow> HandleMapListViewGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshMapList();
private:

	/** Holds the map list. */
	TArray<TSharedPtr<FString> > MapList;

	/** Holds the map list view. */
	TSharedPtr<SListView<TSharedPtr<FString> > > MapListView;

	TSharedPtr<FHotPatcherCookModel> mCookModel;
};

