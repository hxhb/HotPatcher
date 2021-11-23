// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Model/FHotPatcherCreatePatchModel.h"
#include "Cooker/SHotPatcherCookerBase.h"
#include "Cooker/MultiCooker/FMultiCookerSettings.h"

// engine header
#include "Templates/SharedPointer.h"
#include "IStructureDetailsView.h"

/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherMultiCookerPage
	: public SHotPatcherCookerBase
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherMultiCookerPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookerModel> InCreateModel);

public:
	virtual void ImportConfig();
	virtual void ExportConfig()const;
	virtual void ResetConfig();
	virtual void DoGenerate();
	virtual FString GetMissionName() override{return TEXT("MultiCooker");}
	virtual FMultiCookerSettings* GetConfigSettings()override{return CookerSettings.Get();};
protected:
	void CreateExportFilterListView();
	bool CanCook()const;
	FReply RunCook();
	virtual FText GetGenerateTooltipText() const override;
private:

	// TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;

	/** Settings view ui element ptr */
	TSharedPtr<IStructureDetailsView> SettingsView;
	TSharedPtr<FMultiCookerSettings> CookerSettings;
};

