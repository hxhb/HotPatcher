// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Model/FHotPatcherCreatePatchModel.h"
#include "CreatePatch/SHotPatcherPatchableBase.h"
#include "ShaderPatch/FExportShaderPatchSettings.h"

// engine header
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "IStructureDetailsView.h"

/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherExportShaderPatch
	: public SHotPatcherPatchableBase
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherExportShaderPatch) { }
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
	bool CanExportShaderPatch()const;
	FReply DoExportShaderPatch();

private:

	// TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;

	/** Settings view ui element ptr */
	TSharedPtr<IStructureDetailsView> SettingsView;
	TSharedPtr<FExportShaderPatchSettings> ExportShaderPatchSettings;
};

