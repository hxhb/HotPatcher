// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FPatchersModeContext.h"
#include "Cooker/SHotPatcherCookerBase.h"
#include "SHotPatcherPageBase.h"
// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Model/FOriginalCookerContext.h"

/**
 * Implements the profile page for the session launcher wizard.
 */
class SCookersPage
	: public SHotPatcherPageBase
{
public:

	SLATE_BEGIN_ARGS(SCookersPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherContextBase> InContext);
	virtual FString GetPageName() const override{ return TEXT("Cooker"); }
	
public:
	FText HandleCookerModeComboButtonContentText() const;
	void HandleHotPatcherMenuEntryClicked(FString InModeName,TFunction<void(void)> ActionCallback);
	EVisibility HandleOperatorConfigVisibility()const;
	EVisibility HandleImportProjectConfigVisibility()const;
	virtual void SelectToAction(const FString& ActionName);
};
