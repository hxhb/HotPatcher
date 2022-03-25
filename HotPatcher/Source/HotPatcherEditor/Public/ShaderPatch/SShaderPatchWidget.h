// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Model/FPatchersModeContext.h"
#include "SHotPatcherWidgetBase.h"
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
class SShaderPatchWidget
	: public SHotPatcherWidgetBase
{
public:

	SLATE_BEGIN_ARGS(SShaderPatchWidget) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InContext);

public:
	virtual void ImportConfig();
	virtual void ImportProjectConfig(){};
	virtual void ExportConfig()const;
	virtual void ResetConfig();
	virtual void DoGenerate();
	virtual FExportShaderPatchSettings* GetConfigSettings() override{return ExportShaderPatchSettings.Get();};
	virtual FString GetMissionName() override{return TEXT("Shader Patch");}
	virtual FText GetGenerateTooltipText() const override;
protected:
	void CreateExportFilterListView();
	bool CanExportShaderPatch()const;
	bool HasValidConfig()const;
	FReply DoExportShaderPatch();

private:

	// TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;

	/** Settings view ui element ptr */
	TSharedPtr<IStructureDetailsView> SettingsView;
	TSharedPtr<FExportShaderPatchSettings> ExportShaderPatchSettings;
};

