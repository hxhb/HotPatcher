// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FPatchersModeContext.h"
#include "SHotPatcherWidgetBase.h"
#include "SHotPatcherPageBase.h"
// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

/**
 * Implements the profile page for the session launcher wizard.
 */
class SPatchersPage
	: public SHotPatcherPageBase
{
public:

	SLATE_BEGIN_ARGS(SPatchersPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InContext);

public:
	FText HandlePatchModeComboButtonContentText() const;
	void HandleHotPatcherMenuEntryClicked(FString InModeName,TFunction<void(void)> ActionCallback);
	virtual FString GetPageName()const override { return TEXT("Patcher"); };
	EVisibility HandleOperatorConfigVisibility()const;
	EVisibility HandleImportProjectConfigVisibility()const;
	virtual void SelectToAction(const FString& ActionName);

};
