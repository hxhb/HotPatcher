// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Model/FHotPatcherCookModel.h"
#include "SharedPointer.h"
#include "FlibPatchParserHelper.h"
#include "SpecifyCookFilterSetting.h"

// engine header
#include "IDetailsView.h"
#include "SharedPointer.h"
#include "PropertyEditorModule.h"
#include "Kismet/KismetSystemLibrary.h"
/**
 * Implements the cooked Maps panel.
 */
class SHotPatcherCookSpecifyCookFilter
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherCookSpecifyCookFilter) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookModel> InCookModel);

public:
	USpecifyCookFilterSetting* GetSpecifyCookFilterSetting()const;
	TArray<FDirectoryPath> GetAlwayCookDirectory()const;
	TArray<FString> GetAlwayCookAbsDirectory();
	void HandleRequestSpecifyCookFilter(TArray<FDirectoryPath>& OutCookDir);
protected:
	void CreateFilterListView();

private:

	/** Settings view ui element ptr */
	TSharedPtr<IDetailsView> SettingsView;

	USpecifyCookFilterSetting* SpecifyCookFilterSetting;

	TSharedPtr<FHotPatcherCookModel> mCookModel;
};

