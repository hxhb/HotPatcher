// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Model/FPatchersModeContext.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/IPatchableInterface.h"
#include "Model/FHotPatcherContextBase.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "MissionNotificationProxy.h"
#include "PropertyEditorModule.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "Widgets/Text/SMultiLineEditableText.h"

class HOTPATCHEREDITOR_API SHotPatcherWidgetInterface
	: public SCompoundWidget, public IPatchableInterface
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherWidgetInterface) { }
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
class HOTPATCHEREDITOR_API SHotPatcherWidgetBase
	: public SHotPatcherWidgetInterface
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherWidgetBase) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InContext);
	
	virtual void ImportProjectConfig() override;

	virtual FHotPatcherSettingBase* GetConfigSettings(){return nullptr;};
	virtual TSharedPtr<FHotPatcherContextBase> GetContext() { return mContext; }
	virtual void SetContext(TSharedPtr<FHotPatcherContextBase> InContext)
	{
		mContext = InContext;
	}
protected:
	TSharedPtr<FHotPatcherContextBase> mContext;

};

