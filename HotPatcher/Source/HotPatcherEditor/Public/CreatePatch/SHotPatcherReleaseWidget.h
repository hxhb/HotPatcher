// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Model/FPatchersModeContext.h"
#include "SHotPatcherWidgetBase.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/FExportReleaseSettings.h"

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
class SHotPatcherReleaseWidget
	: public SHotPatcherWidgetBase
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherReleaseWidget) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InCreateModel);

public:
	virtual void ImportConfig();
	virtual void ExportConfig()const;
	virtual void ResetConfig();
	virtual void DoGenerate();
	virtual FString GetMissionName() override{return TEXT("Release");}
	virtual FExportReleaseSettings* GetConfigSettings()override{return ExportReleaseSettings.Get();};
protected:
	void CreateExportFilterListView();
	bool CanExportRelease()const;
	FReply DoExportRelease();
	virtual FText GetGenerateTooltipText() const override;
private:

	// TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;

	/** Settings view ui element ptr */
	TSharedPtr<IStructureDetailsView> SettingsView;
	TSharedPtr<FExportReleaseSettings> ExportReleaseSettings;
};

