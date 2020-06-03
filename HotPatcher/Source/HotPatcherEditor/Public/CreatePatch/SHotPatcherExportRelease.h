// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "ExportReleaseSettings.h"
#include "SHotPatcherPatchableBase.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherExportRelease
	: public SHotPatcherPatchableBase
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

public:
	virtual void ImportConfig();
	virtual void ExportConfig()const;
	virtual void ResetConfig();
	virtual void DoGenerate();

protected:
	void CreateExportFilterListView();
	bool CanExportRelease()const;
	FReply DoExportRelease();

private:

	// TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;

	/** Settings view ui element ptr */
	TSharedPtr<IDetailsView> SettingsView;

	UExportReleaseSettings* ExportReleaseSettings;
};

