// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "FHotPatcherVersionControlModel.h"

/**
 * Implements the profile page for the session launcher wizard.
 */
class SProjectVersionControlPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectVersionControlPage) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherVersionControlModel> InVersionControlModel);

private:
	TSharedPtr<FHotPatcherVersionControlModel> mVersionControlModel;
};
