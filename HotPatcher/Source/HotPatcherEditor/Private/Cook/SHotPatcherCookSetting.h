// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "FHotPatcherCookModel.h"
#include "SharedPointer.h"
/**
 * Implements the cooked platforms panel.
 */
class SHotPatcherCookSetting
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SHotPatcherCookSetting) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(	const FArguments& InArgs,TSharedPtr<FHotPatcherCookModel> InCookModel);

protected:



private:


	TSharedPtr<FHotPatcherCookModel> mCookModel;
};

