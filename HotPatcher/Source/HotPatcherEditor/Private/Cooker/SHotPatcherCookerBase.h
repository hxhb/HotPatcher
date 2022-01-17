// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/IPatchableInterface.h"
#include "Model/FHotPatcherCookerModel.h"
// engine header
#include "Cooker/HotPatcherCookerSettingBase.h"
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "MissionNotificationProxy.h"
#include "PropertyEditorModule.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "Widgets/Text/SMultiLineEditableText.h"
/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherCookerBase
	: public SCompoundWidget, public IPatchableInterface
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherCookerBase) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookerModel> InCreateModel);

	virtual void ImportProjectConfig();
	FORCEINLINE virtual void ImportConfig() override {};
	FORCEINLINE virtual void ExportConfig()const override {};
	FORCEINLINE virtual void ResetConfig() override {};
	FORCEINLINE virtual void DoGenerate() override {};

	virtual FPatcherEntitySettingBase* GetConfigSettings(){return nullptr;};
	virtual FString GetMissionName(){return TEXT("");};
	
	virtual FText GetGenerateTooltipText() const;
	TArray<FString> OpenFileDialog()const;
	TArray<FString> SaveFileDialog()const;

protected:
	TSharedPtr<FHotPatcherCookerModel> mCreatePatchModel;

};

