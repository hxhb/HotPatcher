// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "ExportReleaseSettings.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherExportRelease
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherExportRelease) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCreatePatchModel> InCreateModel);

protected:
	void CreateExportFilterListView();
	bool CanExportRelease()const;
	FReply DoExportRelease();

private:

	TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;

	/** Settings view ui element ptr */
	TSharedPtr<IDetailsView> SettingsView;

	UExportReleaseSettings* ExportReleaseSettings;
};

