// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "FHotPatcherCookModel.h"

/**
 * Implements the profile page for the session launcher wizard.
 */
class SProjectCookPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectCookPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookModel> InCookModel);

private:
	TSharedPtr<FHotPatcherCookModel> mCookModel;
};
