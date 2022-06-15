// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "CreatePatch/FExportPatchSettings.h"
#include "CreatePatch/IPatchableInterface.h"
#include "Model/FCookersModeContext.h"
// engine header
#include "Cooker/HotPatcherCookerSettingBase.h"
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "MissionNotificationProxy.h"
#include "PropertyEditorModule.h"
#include "SHotPatcherWidgetBase.h"
#include "ThreadUtils/FProcWorkerThread.hpp"
#include "Widgets/Text/SMultiLineEditableText.h"
/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherCookerBase : public SHotPatcherWidgetInterface
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
	FORCEINLINE void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InCreateModel)
	{
		mCreatePatchModel = InCreateModel;
	}


protected:
	FCookersModeContext* GetCookerModelPtr()const { return (FCookersModeContext*)mCreatePatchModel.Get(); }
	TSharedPtr<FHotPatcherContextBase> mCreatePatchModel;

};

