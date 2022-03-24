// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Model/FHotPatcherCreatePatchModel.h"
#include "Cooker/SHotPatcherCookerBase.h"

// engine header
#include "Interfaces/ITargetPlatform.h"
#include "Templates/SharedPointer.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Model/FHotPatcherOriginalCookerModel.h"

/**
 * Implements the profile page for the session launcher wizard.
 */
class SProjectCookerPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectCookerPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherModelBase> InCreatePatchModel);

public:
	FText HandleCookerModeComboButtonContentText() const;
	void HandleHotPatcherMenuEntryClicked(FString InModeName,TFunction<void(void)> ActionCallback);

	EVisibility HandleOperatorConfigVisibility()const;
	EVisibility HandleImportProjectConfigVisibility()const;
protected:
	FReply DoExportConfig()const;
	FReply DoImportConfig()const;
	FReply DoImportProjectConfig()const;
	FReply DoResetConfig()const;

	TSharedPtr<IPatchableInterface> GetActivePatchable()const;

private:
	TSharedPtr<FHotPatcherOriginalCookerModel> OriginalCookModel;
private:
	FORCEINLINE FHotPatcherCookerModel* GetCookModelPtr()const
	{
		return (FHotPatcherCookerModel*)mCreateCookerModel.Get();
	}
	TSharedPtr<FHotPatcherModelBase> mCreateCookerModel;
	// TSharedPtr<SHotPatcherCookerBase> mOriginal;
	// TSharedPtr<SHotPatcherCookerBase> mMultiProcess;
	TMap<FString,TSharedRef<SHotPatcherCookerBase>> CookActionMaps;
};
