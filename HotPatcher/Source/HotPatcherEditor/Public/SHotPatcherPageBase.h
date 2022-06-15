// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FPatchersModeContext.h"
#include "SHotPatcherWidgetBase.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

/**
 * Implements the profile page for the session launcher wizard.
 */
class SHotPatcherPageBase
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherPageBase) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InContext)
	{
		SetContext(InContext);
	}
	virtual void SetContext(TSharedPtr<FHotPatcherContextBase> InContext)
	{
		Context = InContext;
	}
	
	FReply DoExportConfig()const;
	FReply DoImportConfig()const;
	FReply DoImportProjectConfig()const;
	FReply DoResetConfig()const;

	virtual TSharedPtr<FHotPatcherContextBase> GetContext()const { return Context; }
	virtual TMap<FName,TSharedPtr<SHotPatcherWidgetBase>> GetActionWidgetMap()const { return ActionWidgetMap; }

	TSharedPtr<SHotPatcherWidgetBase> GetActiveAction()const;
protected:
	TSharedPtr<FHotPatcherContextBase> Context;
	TMap<FName,TSharedPtr<SHotPatcherWidgetBase>> ActionWidgetMap;
};
