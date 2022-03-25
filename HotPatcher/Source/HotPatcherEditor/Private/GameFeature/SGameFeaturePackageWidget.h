// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Model/FPatchersModeContext.h"
#include "SHotPatcherWidgetBase.h"
#include "GameFeature/FGameFeaturePackagerSettings.h"

// engine header
#include "Templates/SharedPointer.h"
#include "IStructureDetailsView.h"

/**
 * Implements the cooked platforms panel.
 */
class SGameFeaturePackageWidget
	: public SHotPatcherWidgetBase
{
public:

	SLATE_BEGIN_ARGS(SGameFeaturePackageWidget) { }
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
	virtual void ImportProjectConfig(){};
	virtual void ExportConfig()const;
	virtual void ResetConfig();
	virtual void DoGenerate();
	
	virtual FGameFeaturePackagerSettings* GetConfigSettings() override{return GameFeaturePackagerSettings.Get();};
	virtual FGameFeaturePackagerSettings* GetConfigSettings()const {return GameFeaturePackagerSettings.Get();};
	
	virtual FString GetMissionName() override{return TEXT("Game Feature Packager");}
	virtual FText GetGenerateTooltipText() const override;
protected:
	void CreateExportFilterListView();
	bool CanGameFeaturePackager()const;
	bool HasValidConfig()const;
	FReply DoGameFeaturePackager();
	void FeaturePackager();

private:

	// TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;
	TSharedPtr<FExportPatchSettings> PatchSettings;
	/** Settings view ui element ptr */
	TSharedPtr<IStructureDetailsView> SettingsView;
	TSharedPtr<FGameFeaturePackagerSettings> GameFeaturePackagerSettings;
};

