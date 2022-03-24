// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "SHotPatcherInformations.h"
#include "CreatePatch/IPatchableInterface.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "MissionNotificationProxy.h"
#include "PropertyEditorModule.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "Widgets/Text/SMultiLineEditableText.h"

class HOTPATCHEREDITOR_API SHotPatcherPatchableInterface
	: public SCompoundWidget, public IPatchableInterface
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherPatchableInterface) { }
	SLATE_END_ARGS()

	virtual void ImportProjectConfig(){};
	FORCEINLINE virtual void ImportConfig() {};
	FORCEINLINE virtual void ExportConfig()const {};
	FORCEINLINE virtual void ResetConfig() {};
	FORCEINLINE virtual void DoGenerate() {};
	virtual FPatcherEntitySettingBase* GetConfigSettings(){return nullptr;};
	virtual FString GetMissionName(){return TEXT("");};
	
	virtual FText GetGenerateTooltipText() const;
	virtual TArray<FString> OpenFileDialog()const;
	virtual TArray<FString> SaveFileDialog()const;
};
/**
 * Implements the cooked platforms panel.
 */
class HOTPATCHEREDITOR_API SHotPatcherPatchableBase
	: public SHotPatcherPatchableInterface
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherPatchableBase) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherModelBase> InCreateModel);
	
	virtual void ImportProjectConfig() override;
	// FORCEINLINE virtual void ImportConfig() override {};
	// FORCEINLINE virtual void ExportConfig()const override {};
	// FORCEINLINE virtual void ResetConfig() override {};
	// FORCEINLINE virtual void DoGenerate() override {};

	virtual FHotPatcherSettingBase* GetConfigSettings(){return nullptr;};


protected:
	// FHotPatcherCreatePatchModel* GetPatchModel()const { return (FHotPatcherCreatePatchModel*)mCreatePatchModel.Get(); }
	TSharedPtr<FHotPatcherModelBase> mCreatePatchModel;

};

