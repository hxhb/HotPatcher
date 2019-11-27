// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "FHotPatcherCreatePatchModel.h"

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

private:
	TSharedPtr<FHotPatcherCreatePatchModel> mCreatePatchModel;
};
