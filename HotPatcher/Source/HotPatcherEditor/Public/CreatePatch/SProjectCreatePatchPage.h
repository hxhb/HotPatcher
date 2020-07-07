// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "SHotPatcherPatchableBase.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"

/**
 * Implements the profile page for the session launcher wizard.
 */
class SProjectCreatePatchPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectCreatePatchPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCreatePatchModel> InCreatePatchModel);

public:
	FText HandlePatchModeComboButtonContentText() const;
	void HandleHotPatcherMenuEntryClicked(EHotPatcherActionModes::Type InMode);
	EVisibility HandleExportPatchVisibility() const;
	EVisibility HandleExportReleaseVisibility() const;

	EVisibility HandleOperatorConfigVisibility()const;
protected:
	FReply DoExportConfig()const;
	FReply DoImportConfig()const;
	FReply DoResetConfig()const;

	TSharedPtr<IPatchableInterface> GetActivePatchable()const;
private:
	TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;
	TSharedPtr<SHotPatcherPatchableBase> mPatch;
	TSharedPtr<SHotPatcherPatchableBase> mRelease;
};
